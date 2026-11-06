#!/usr/bin/env python3
'''
Generate a static HTML status page for your open GitHub PRs.

Run from cron. "Dealt with" ticks are kept in the browser's localStorage,
along with an activity fingerprint, so a tick clears itself when the PR
changes. Closed/merged PRs drop out because only open PRs are fetched.
'''

import argparse
import fcntl
import fnmatch
import json
import os
import re
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
USER_CONFIG = os.path.join(os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config"),
                           "pr_status", "config.json")

DEFAULTS = {
    "github_user": "",
    "searches": ["author:{user}"],
    "include_repos": [],
    "exclude_repos": [],
    "ignore_users": [],
    "ignore_bots": True,
    "reset_on": ["activity", "ci_failure", "branch_ci_failure", "review_decision", "conflict", "apreview"],
    "output": "pr_status.html",
    "title": "",
    "refresh_seconds": 300,
    "stale_minutes": 60,
    "token_file": "",
    "page_size": 25,
    "apreview": None,
    "ci_status": "",
    "devcall": {},
}

PR_FIELDS = '''
fragment pr on PullRequest {
        id number title url state isDraft createdAt updatedAt mergeable reviewDecision headRefOid body
        repository { nameWithOwner isArchived }
        isCrossRepository headRefName headRepository { nameWithOwner }
        headRef { target { ... on Commit { statusCheckRollup { state } } } }
        author { login }
        labels(first: 10) { nodes { name } }
        commits(last: 1) { nodes { commit { committedDate statusCheckRollup { state } } } }
        latestReviews(first: 20) { nodes { author { login } state } }
        reviewRequests(first: 10) {
          nodes { requestedReviewer { __typename ... on User { login } ... on Team { name } ... on Bot { login } } }
        }
        comments(last: 30) { totalCount nodes { id author { login __typename } createdAt } }
@@PROJECT@@
        reviews(last: 30) { totalCount nodes { author { login __typename } state submittedAt comments { totalCount } } }
}
'''

# needs the read:project token scope, so only asked for when an APReview project is configured
PROJECT_QUERY = '''
        projectItems(first: 10) {
          nodes {
            project { number owner { ... on Organization { login } ... on User { login } } }
            fieldValueByName(name: $field) { ... on ProjectV2ItemFieldSingleSelectValue { name updatedAt } }
          }
        }'''

VIEWER_QUERY = 'query { viewer { login } }'

QUERY = '''
query($q: String!, $n: Int!, $after: String@@FIELDVAR@@) {
  rateLimit { cost remaining resetAt }
  search(query: $q, type: ISSUE, first: $n, after: $after) {
    issueCount
    pageInfo { hasNextPage endCursor }
    nodes { ...pr }
  }
}
''' + PR_FIELDS

PR_NODES_QUERY = '''
query($ids: [ID!]!@@FIELDVAR@@) {
  nodes(ids: $ids) { ...pr }
}
''' + PR_FIELDS

# a PR that search misses is still looked up directly for this long after search last found it
KEEP_SECONDS = 2 * 86400

COMMENT_QUERY = '''
query($ids: [ID!]!) {
  nodes(ids: $ids) { ... on IssueComment { id body } }
}
'''

# A dependency is named by one of these phrases at the start of a line or
# sentence, followed straight away by the references, e.g. "Depends on #123",
# "**Requires:** #1 and ArduPilot/mavlink#5", "- Needs PR #7", or by a bare
# "Depends on:" with a list of them below. Phrases anywhere else are prose
# ("nothing here depends on #5", "doesn't need #6") and don't count.
DEP_RE = re.compile(r'(?:^|[.!?][*_]*\s+)[\s>*_+#-]*(?:\[[ xX]\]\s*)?(?:\d+[.)]\s*)?[*_]*'
                    r'(?:(?:this(?:\s+(?:pr|branch|change|patch|series))?|it)\s+(?:is\s+)?)?'
                    r'(?:depend(?:s|ing|ent)?\s+(?:up)?on|requires|needs|based\s+on|builds?\s+on|built\s+on|'
                    r'on\s+top\s+of|blocked\s+by|waiting\s+(?:for|on))\b[\s:*_]*', re.I)
REF_RE = re.compile(r'(?:(?:a|an|the)\s+)?(?:(?:PRs?|pull\s+requests?|issues?)\s*)?'
                    # a URL, maybe a Markdown link or <autolink> and maybe to the files or commits tab
                    r'(?:(?:\[[^\]]*\]\(|<)?https://github\.com/([\w.-]+)/([\w.-]+)/(?:pull|issues)/(\d+)'
                    r'(?:/[\w/]*|#\S*)?(?<=\S)[)>]?(?=[\s,.;:)]|$)'
                    r'|([\w.-]+)/([\w.-]+)#(\d+)(?![\w/-])'
                    r'|#(\d+)(?![\w/-]))', re.I)
# between the references of a run: "#1, #2 and #3"
REF_SEP_RE = re.compile(r'[\s,]*(?:(?:and|&|\+)\s+)?')
LIST_ITEM_RE = re.compile(r'\s*(?:[-*+]|\d+[.)])\s+(?:\[[ xX]\]\s*)?[*_]*')
# HTML comments (PR templates) and code blocks
NOT_TEXT_RE = re.compile(r'<!--.*?-->|```.*?```', re.S)

VERDICT_RE = re.compile(r'\*\*Verdict:\s*([A-Z _-]+?)\s*\*\*')
REPORT_RE = re.compile(r'Full report:\s*(\S+)')
HEAD_RE = re.compile(r'Reviewed at head `([0-9a-f]+)`')


def log(msg):
    print(f"{time.strftime('%Y-%m-%d %H:%M:%S')} {msg}", file=sys.stderr)


def find_config(path):
    if path:
        return path
    for p in (os.environ.get("PR_STATUS_CONFIG"), USER_CONFIG, os.path.join(SCRIPT_DIR, "config.json")):
        if p and os.path.exists(p):
            return p
    raise SystemExit(f"no config found: run '{sys.argv[0]} --init' to create {USER_CONFIG}")


def load_config(path):
    with open(path) as f:
        cfg = dict(DEFAULTS, **json.load(f))
    # relative paths are relative to the config file
    base = os.path.dirname(os.path.abspath(path))
    for k in ("output", "token_file"):
        if cfg[k]:
            cfg[k] = os.path.join(base, os.path.expanduser(cfg[k]))
    ar = cfg["apreview"]
    if ar:
        ar.setdefault("user", "")
        ar.setdefault("field", "Result")
        ar["project"] = bool(ar.get("project_owner") and ar.get("project_number"))
    return cfg


def get_token(cfg):
    for var in ("GH_TOKEN", "GITHUB_TOKEN"):
        if os.environ.get(var):
            return os.environ[var].strip()
    if cfg["token_file"]:
        with open(cfg["token_file"]) as f:
            return f.read().strip()
    try:
        return subprocess.run(["gh", "auth", "token"], capture_output=True, text=True,
                              check=True, timeout=30).stdout.strip()
    except (OSError, subprocess.SubprocessError) as e:
        raise SystemExit(f"no GitHub token: set GH_TOKEN, token_file, or log in with gh ({e})")


def graphql(token, variables, query, quiet=False):
    body = json.dumps({"query": query, "variables": variables}).encode()
    for attempt in range(4):
        req = urllib.request.Request("https://api.github.com/graphql", data=body, headers={
            "Authorization": f"bearer {token}",
            "Content-Type": "application/json",
            "User-Agent": "pr_status.py",
        })
        try:
            with urllib.request.urlopen(req, timeout=90) as r:
                res = json.load(r)
        except (urllib.error.URLError, TimeoutError) as e:
            # search with nested connections gives occasional 502s
            if attempt == 3:
                raise
            log(f"graphql retry after: {e}")
            time.sleep(5 * (attempt + 1))
            continue
        if res.get("errors"):
            if not res.get("data"):
                raise RuntimeError(f"graphql errors: {res['errors']}")
            if not quiet:
                log(f"graphql partial errors: {res['errors']}")
        return res["data"]


def viewer_login(token):
    return graphql(token, {}, VIEWER_QUERY)["viewer"]["login"]


def pr_query(cfg, query, variables):
    '''query and variables, with the project fields only when an APReview project is configured'''
    ar = cfg["apreview"]
    project = bool(ar and ar["project"])
    query = query.replace("@@FIELDVAR@@", ", $field: String!" if project else "")
    query = query.replace("@@PROJECT@@", PROJECT_QUERY if project else "")
    if project:
        variables = dict(variables, field=ar["field"])
    return variables, query


def search(cfg, token, q):
    found = {}
    after = None
    while True:
        data = graphql(token, *pr_query(cfg, QUERY, {"q": q, "n": cfg["page_size"], "after": after}))
        s = data["search"]
        for node in s["nodes"]:
            if node and node.get("url"):
                found[node["url"]] = node
        if not s["pageInfo"]["hasNextPage"]:
            break
        after = s["pageInfo"]["endCursor"]
    if s["issueCount"] > 1000:
        log(f"warning: '{q}' matches {s['issueCount']} PRs, search only returns 1000")
    return found


def known_path(cfg):
    d, name = os.path.split(cfg["output"])
    return os.path.join(d, f".{name}.known.json")


def load_known(cfg):
    '''url -> {"id", "found"} for the PRs on the last page written'''
    try:
        with open(known_path(cfg)) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def fetch_prs(cfg, token, known):
    '''open PRs, and the known dict to save once the page is written'''
    prs = {}
    for s in cfg["searches"]:
        q = f"is:pr is:open archived:false sort:updated-desc {s.format(user=cfg['github_user'])}"
        prs.update(search(cfg, token, q))
    now = datetime.now(timezone.utc)
    stamp = now.strftime("%Y-%m-%dT%H:%M:%SZ")
    new_known = {u: {"id": pr["id"], "found": stamp} for u, pr in prs.items()}

    # search sometimes comes back short or empty without an error, which would
    # drop PRs from the page and lose their ticks, so look up the ones it
    # missed directly and keep any that are still open
    def recent(k):
        found = datetime.strptime(k["found"], "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
        return (now - found).total_seconds() < KEEP_SECONDS
    missed = {k["id"]: k for u, k in known.items() if u not in prs and recent(k)}
    ids = list(missed)
    kept = 0
    for i in range(0, len(ids), cfg["page_size"]):
        data = graphql(token, *pr_query(cfg, PR_NODES_QUERY, {"ids": ids[i:i+cfg["page_size"]]}))
        for node in data["nodes"]:
            if node and node.get("state") == "OPEN" and not node["repository"]["isArchived"]:
                prs[node["url"]] = node
                new_known[node["url"]] = missed[node["id"]]
                kept += 1
    if kept:
        log(f"search missed {kept} open PRs, kept them from the last run")
    return list(prs.values()), new_known


def fetch_apreview_comments(cfg, token, prs):
    '''bodies of the newest AP-Review comment on each PR, keyed by comment id'''
    if not cfg["apreview"] or not cfg["apreview"]["user"]:
        return {}
    bot = cfg["apreview"]["user"].lower()
    ids = []
    for pr in prs:
        mine = [c for c in pr["comments"]["nodes"] if ((c["author"] or {}).get("login") or "").lower() == bot]
        if mine:
            ids.append(max(mine, key=lambda c: c["createdAt"])["id"])
    bodies = {}
    for i in range(0, len(ids), 50):
        for node in graphql(token, {"ids": ids[i:i+50]}, COMMENT_QUERY)["nodes"]:
            if node:
                bodies[node["id"]] = node["body"]
    return bodies


def fetch_devcall(cfg):
    '''URLs of the items matching the devcall filter, or None if there's no answer'''
    dc = cfg["devcall"] or {}
    if not dc:
        return None
    url = (f"https://api.github.com/orgs/{dc['project_owner']}/projectsV2/{dc['project_number']}/items?"
           + urllib.parse.urlencode({"q": dc["filter"], "per_page": 100}))
    urls = set()
    try:
        while url:
            # no token: a public project reads fine anonymously, while a token
            # without org Projects access gets nothing back
            req = urllib.request.Request(url, headers={
                "Accept": "application/vnd.github+json",
                "User-Agent": "pr_status.py",
            })
            with urllib.request.urlopen(req, timeout=30) as r:
                items = json.load(r)
                m = re.search(r'<([^>]+)>;\s*rel="next"', r.headers.get("Link", ""))
            if not isinstance(items, list):
                raise ValueError(f"unexpected reply: {str(items)[:200]}")
            urls.update((i.get("content") or {}).get("html_url") for i in items)
            url = m.group(1) if m else None
    except (OSError, ValueError) as e:
        # not fatal: the page is written without the marks
        log(f"devcall lookup failed: {e}")
        return None
    urls.discard(None)
    return urls


def ref_run(text, pos, own):
    '''the (owner, repo, number) references in a run starting at text[pos]'''
    refs = []
    while True:
        r = REF_RE.match(text, pos)
        if not r:
            return refs
        if r.group(3):
            refs.append((r.group(1), r.group(2), int(r.group(3))))
        elif r.group(6):
            refs.append((r.group(4), r.group(5), int(r.group(6))))
        else:
            refs.append(own + (int(r.group(7)),))
        pos = REF_SEP_RE.match(text, r.end()).end()


def pr_deps(pr):
    '''(owner, repo, number) of each dependency named in the PR body, in order'''
    own = tuple(pr["repository"]["nameWithOwner"].split("/"))
    lines = NOT_TEXT_RE.sub("", pr.get("body") or "").splitlines()
    found = []
    for i, line in enumerate(lines):
        for m in DEP_RE.finditer(line):
            refs = ref_run(line, m.end(), own)
            if not refs and m.end() == len(line.rstrip()):
                # "Depends on:" with a list below it, one reference at the start of each item
                for nxt in lines[i+1:]:
                    item = LIST_ITEM_RE.match(nxt)
                    if not item:
                        if not nxt.strip() and not refs:
                            continue
                        break
                    refs += ref_run(nxt, item.end(), own)[:1]
            for ref in refs:
                key = dep_key(ref)
                if key != dep_key(own + (pr["number"],)) and key not in {dep_key(f) for f in found}:
                    found.append(ref)
    return found


def dep_key(ref):
    owner, repo, number = ref
    return f"{owner}/{repo}#{number}".lower()


def fetch_deps(cfg, token, prs):
    '''dep_key -> {"state", "url"} for the dependencies of these PRs, or None if
    the lookup failed. One that doesn't exist or can't be read is left out.'''
    if "depends" not in cfg["reset_on"]:
        return {}
    refs = {}
    for pr in prs:
        for ref in pr_deps(pr):
            refs.setdefault(dep_key(ref), ref)
    keys = sorted(refs)
    states = {}
    try:
        for i in range(0, len(keys), 50):
            # one aliased lookup per dependency; numbers can be PRs or issues
            fields = []
            for j, key in enumerate(keys[i:i+50]):
                owner, repo, number = refs[key]
                fields.append(f'd{j}: repository(owner: {json.dumps(owner)}, name: {json.dumps(repo)}) {{ '
                              f'issueOrPullRequest(number: {number}) {{ '
                              f'... on PullRequest {{ state url }} ... on Issue {{ state url }} }} }}')
            # a typo in someone's PR body would otherwise log an error on every run
            data = graphql(token, {}, "query {\n" + "\n".join(fields) + "\n}", quiet=True)
            for j, key in enumerate(keys[i:i+50]):
                node = ((data.get(f"d{j}") or {}).get("issueOrPullRequest")) or {}
                if node.get("state"):
                    states[key] = {"state": node["state"], "url": node["url"]}
    except (OSError, RuntimeError, ValueError) as e:
        # not fatal: the page is written without dependency states
        log(f"dependency lookup failed: {e}")
        return None
    return states


def apreview_status(cfg, pr, bodies):
    ar = cfg["apreview"]
    bot = ar["user"].lower()

    def norm(v):
        return re.sub(r'[\s_-]+', ' ', v).strip().upper()

    proj = None
    for item in (pr.get("projectItems") or {}).get("nodes", []):
        # items in projects the token can't see come back as null, without an error
        if not item or not item.get("project"):
            continue
        owner = (item["project"].get("owner") or {}).get("login", "")
        val = item.get("fieldValueByName")
        if item["project"]["number"] == ar["project_number"] and owner.lower() == ar["project_owner"].lower() and val:
            proj = {"status": norm(val["name"]), "at": val["updatedAt"]}

    com = None
    mine = [c for c in pr["comments"]["nodes"] if bot and ((c["author"] or {}).get("login") or "").lower() == bot]
    if mine:
        c = max(mine, key=lambda c: c["createdAt"])
        body = bodies.get(c["id"], "")
        m = VERDICT_RE.search(body)
        if m:
            report = REPORT_RE.search(body)
            head = HEAD_RE.search(body)
            com = {"status": norm(m.group(1)), "at": c["createdAt"],
                   "url": report.group(1) if report else "", "head": head.group(1) if head else ""}

    if not proj and not com:
        return {"status": "", "url": "", "head_stale": False, "at": ""}
    # the newer of the two wins; the comment still supplies the report link and reviewed head
    best = proj if not com or (proj and proj["at"] > com["at"]) else com
    com = com or {}
    head = com.get("head", "")
    url = com.get("url", "")
    if not url and ar["project"]:
        url = f"https://github.com/orgs/{ar['project_owner']}/projects/{ar['project_number']}"
    return {
        "status": best["status"],
        "at": best["at"],
        "url": url or pr["url"],
        "head_stale": bool(head) and not pr["headRefOid"].startswith(head),
    }


def repo_wanted(cfg, repo):
    if cfg["include_repos"] and not any(fnmatch.fnmatch(repo, p) for p in cfg["include_repos"]):
        return False
    return not any(fnmatch.fnmatch(repo, p) for p in cfg["exclude_repos"])


def summarise(cfg, pr, bodies, devcall, deps):
    me = cfg["github_user"].lower()
    ignored = {u.lower() for u in cfg["ignore_users"]}

    def who(author):
        return (author or {}).get("login") or "ghost"

    def is_other(author):
        login = who(author).lower()
        if login == me or login in ignored:
            return False
        bot = (author or {}).get("__typename") == "Bot" or login.endswith("[bot]")
        return not (bot and cfg["ignore_bots"])

    events = []
    my_times = []
    for c in pr["comments"]["nodes"]:
        if is_other(c["author"]):
            events.append([c["createdAt"], who(c["author"]), "comment"])
        elif who(c["author"]).lower() == me:
            my_times.append(c["createdAt"])
    ncomments = pr["comments"]["totalCount"]
    for r in pr["reviews"]["nodes"]:
        ncomments += r["comments"]["totalCount"]
        if not r["submittedAt"]:
            continue
        if is_other(r["author"]):
            kind = {"APPROVED": "approved", "CHANGES_REQUESTED": "changes requested"}.get(r["state"], "review")
            events.append([r["submittedAt"], who(r["author"]), kind])
        elif who(r["author"]).lower() == me:
            my_times.append(r["submittedAt"])
    events.sort(reverse=True)

    commit = (pr["commits"]["nodes"] or [{}])[0].get("commit") or {}
    if commit.get("committedDate"):
        my_times.append(commit["committedDate"])
    ci = (commit.get("statusCheckRollup") or {}).get("state") or "NONE"
    # CI that ran in the head repo itself, e.g. on push to a fork; None when the
    # branch is in the PR's own repo, the fork or branch is gone, or no CI ran there
    head_repo = (pr["headRepository"] or {}).get("nameWithOwner", "")
    branch_ci = None
    if pr["isCrossRepository"] and head_repo and pr["headRef"]:
        branch_ci = ((pr["headRef"]["target"] or {}).get("statusCheckRollup") or {}).get("state")

    latest = pr["latestReviews"]["nodes"]
    changes_by = sorted(who(r["author"]) for r in latest if r["state"] == "CHANGES_REQUESTED")
    approved_by = sorted(who(r["author"]) for r in latest if r["state"] == "APPROVED")
    requested = []
    for rr in pr["reviewRequests"]["nodes"]:
        rv = rr["requestedReviewer"] or {}
        requested.append(rv.get("login") or rv.get("name") or "?")

    # what clears a tick: each part is compared separately so the page can say why
    parts = {}
    reset_on = cfg["reset_on"]
    if "activity" in reset_on:
        parts["activity"] = events[0][0] if events else ""
    if "ci_failure" in reset_on:
        parts["ci_failure"] = ci in ("FAILURE", "ERROR")
    if "ci" in reset_on:
        parts["ci"] = ci
    if "branch_ci_failure" in reset_on:
        parts["branch_ci_failure"] = None if branch_ci is None else branch_ci in ("FAILURE", "ERROR")
    if "branch_ci" in reset_on:
        parts["branch_ci"] = branch_ci
    if "review_decision" in reset_on:
        parts["review_decision"] = pr["reviewDecision"] or ""
    if "conflict" in reset_on:
        # GitHub reports UNKNOWN while it recomputes, e.g. after the base branch moves
        parts["conflict"] = None if pr["mergeable"] == "UNKNOWN" else pr["mergeable"] == "CONFLICTING"
    apreview = apreview_status(cfg, pr, bodies) if cfg["apreview"] else None
    if "apreview" in reset_on and apreview:
        parts["apreview"] = apreview["status"]
    if "push" in reset_on:
        parts["push"] = pr["headRefOid"]
    # None when the lookup failed; the page then neither keeps nor clears ticks on it
    ready = None if devcall is None else pr["url"] in devcall
    if "devcall" in reset_on:
        parts["devcall"] = ready
    depends = []
    if "depends" in reset_on:
        for ref in pr_deps(pr):
            owner, repo, number = ref
            got = (deps or {}).get(dep_key(ref))
            same = f"{owner}/{repo}".lower() == pr["repository"]["nameWithOwner"].lower()
            depends.append({
                "key": dep_key(ref),
                "ref": f"#{number}" if same else f"{owner}/{repo}#{number}",
                # GitHub redirects an issues URL to the PR
                "url": got["url"] if got else f"https://github.com/{owner}/{repo}/issues/{number}",
                "state": got["state"] if got else None,
            })
        # the dependencies no longer open; the page only clears a tick when one is added
        parts["depends"] = None if deps is None else [d["key"] for d in depends if d["state"] in ("MERGED", "CLOSED")]

    return {
        "url": pr["url"],
        "repo": pr["repository"]["nameWithOwner"],
        "number": pr["number"],
        "title": pr["title"],
        "author": who(pr["author"]),
        "draft": pr["isDraft"],
        "labels": [l["name"] for l in pr["labels"]["nodes"]],
        "created": pr["createdAt"],
        "updated": pr["updatedAt"],
        "ci": ci,
        "branch_ci": branch_ci,
        "head_repo": head_repo,
        "branch": pr["headRefName"],
        "branch_tree_url": (f"https://github.com/{head_repo}/tree/"
                            + urllib.parse.quote(pr["headRefName"], safe="/")) if head_repo else "",
        "branch_url": (f"https://github.com/{head_repo}/actions?query="
                       + urllib.parse.quote(f"branch:{pr['headRefName']}", safe="")) if branch_ci else "",
        "mergeable": pr["mergeable"],
        "review": pr["reviewDecision"] or "",
        "changes_by": changes_by,
        "approved_by": approved_by,
        "requested": requested,
        "ncomments": ncomments,
        "events": events[:50],
        "my_last": max(my_times) if my_times else "",
        "apreview": apreview,
        "devcall": ready,
        "depends": depends,
        "parts": parts,
    }


def write_atomic(path, text):
    d = os.path.dirname(path) or "."
    os.makedirs(d, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=d, prefix=".pr_status.")
    try:
        with os.fdopen(fd, "w") as f:
            f.write(text)
        os.chmod(tmp, 0o644)
        os.replace(tmp, path)
    except BaseException:
        os.unlink(tmp)
        raise


def render(cfg, prs, devcall, deps):
    dc = cfg["devcall"] or {}
    data = {
        "generated": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "user": cfg["github_user"],
        "apreview": bool(cfg["apreview"]),
        "refresh_seconds": cfg["refresh_seconds"],
        "stale_minutes": cfg["stale_minutes"],
        "ci_status": cfg["ci_status"],
        "prs": prs,
        "devcall": {
            "label": dc.get("label", "ReadyForDevCall"),
            "url": f"https://github.com/orgs/{dc['project_owner']}/projects/{dc['project_number']}" if dc else "",
            "failed": bool(dc) and devcall is None,
        },
        "depends_failed": deps is None,
    }
    # no '<' at all, so nothing in the data can end or confuse the script element
    blob = json.dumps(data).replace("<", "\\u003c")
    with open(os.path.join(SCRIPT_DIR, "template.html")) as f:
        tpl = f.read()
    title = (cfg["title"] or f"{cfg['github_user']} PR Status").replace("&", "&amp;").replace("<", "&lt;")
    return tpl.replace("@@TITLE@@", title).replace("@@DATA@@", blob)


def init_config(path, cfg):
    '''write a starter config, filling in the token's login'''
    path = path or USER_CONFIG
    if os.path.exists(path):
        raise SystemExit(f"{path} already exists")
    user = viewer_login(get_token(cfg))
    with open(os.path.join(SCRIPT_DIR, "config.example.json")) as f:
        new = json.load(f)
    new["github_user"] = user
    new["output"] = os.path.join(os.path.dirname(os.path.abspath(path)), "pr_status.html")
    write_atomic(path, json.dumps(new, indent=4) + "\n")
    print(f"wrote {path} for {user}; the page will be {new['output']}")
    print(f"cron: */15 * * * * {sys.executable} {os.path.abspath(__file__)}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("-c", "--config",
                    help=f"config file (default: $PR_STATUS_CONFIG, {USER_CONFIG}, or config.json next to the script)")
    ap.add_argument("--init", action="store_true", help="create a starter config and exit")
    ap.add_argument("--json", help="also write the PR data to this file")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if args.init:
        init_config(args.config, dict(DEFAULTS))
        return

    cfg = load_config(find_config(args.config))

    # skip if the previous cron run is still going
    lock = open(cfg["output"] + ".lock", "w")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        log("previous run still active, skipping")
        return

    token = get_token(cfg)
    if not cfg["github_user"]:
        cfg["github_user"] = viewer_login(token)
    raw, known = fetch_prs(cfg, token, load_known(cfg))
    raw = [pr for pr in raw if repo_wanted(cfg, pr["repository"]["nameWithOwner"])]
    known = {u: k for u, k in known.items() if u in {pr["url"] for pr in raw}}
    bodies = fetch_apreview_comments(cfg, token, raw)
    devcall = fetch_devcall(cfg)
    deps = fetch_deps(cfg, token, raw)
    prs = [summarise(cfg, pr, bodies, devcall, deps) for pr in raw]
    write_atomic(cfg["output"], render(cfg, prs, devcall, deps))
    write_atomic(known_path(cfg), json.dumps(known))
    if args.json:
        write_atomic(args.json, json.dumps(prs, indent=1))
    if args.verbose:
        ndev = "off" if not cfg["devcall"] else "failed" if devcall is None else sum(1 for p in prs if p["devcall"])
        ndep = "off" if "depends" not in cfg["reset_on"] else "failed" if deps is None else len(deps)
        log(f"{len(prs)} PRs, {len(bodies)} AP-Review comments, devcall {ndev}, {ndep} dependencies"
            f" -> {cfg['output']}")


if __name__ == "__main__":
    main()
