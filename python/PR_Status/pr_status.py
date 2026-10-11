#!/usr/bin/env python3
'''
Generate a static HTML status page for my open GitHub PRs.

Run from cron. "Dealt with" ticks are kept in the browser's localStorage,
keyed on an activity fingerprint, so a tick clears itself when the PR
changes. Closed/merged PRs drop out because only open PRs are fetched.
'''

import argparse
import fcntl
import fnmatch
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

DEFAULTS = {
    "github_user": "",
    "searches": ["author:{user}"],
    "include_repos": [],
    "exclude_repos": [],
    "ignore_users": [],
    "ignore_bots": True,
    "reset_on": ["activity", "ci_failure", "review_decision", "conflict", "apreview"],
    "output": os.path.join(SCRIPT_DIR, "pr_status.html"),
    "title": "PR Status",
    "refresh_seconds": 300,
    "stale_minutes": 60,
    "token_file": "",
    "page_size": 25,
    "apreview": {"user": "AP-Review", "project_owner": "ArduPilot", "project_number": 33, "field": "Result"},
}

QUERY = '''
query($q: String!, $n: Int!, $after: String, $field: String!) {
  rateLimit { cost remaining resetAt }
  search(query: $q, type: ISSUE, first: $n, after: $after) {
    issueCount
    pageInfo { hasNextPage endCursor }
    nodes {
      ... on PullRequest {
        number title url isDraft createdAt updatedAt mergeable reviewDecision headRefOid
        repository { nameWithOwner }
        author { login }
        labels(first: 10) { nodes { name } }
        commits(last: 1) { nodes { commit { committedDate statusCheckRollup { state } } } }
        latestReviews(first: 20) { nodes { author { login } state } }
        reviewRequests(first: 10) {
          nodes { requestedReviewer { __typename ... on User { login } ... on Team { name } ... on Bot { login } } }
        }
        comments(last: 30) { totalCount nodes { id author { login __typename } createdAt } }
        projectItems(first: 10) {
          nodes {
            project { number owner { ... on Organization { login } ... on User { login } } }
            fieldValueByName(name: $field) { ... on ProjectV2ItemFieldSingleSelectValue { name updatedAt } }
          }
        }
        reviews(last: 30) { totalCount nodes { author { login __typename } state submittedAt comments { totalCount } } }
      }
    }
  }
}
'''

COMMENT_QUERY = '''
query($ids: [ID!]!) {
  nodes(ids: $ids) { ... on IssueComment { id body } }
}
'''

VERDICT_RE = re.compile(r'\*\*Verdict:\s*([A-Z _-]+?)\s*\*\*')
REPORT_RE = re.compile(r'Full report:\s*(\S+)')
HEAD_RE = re.compile(r'Reviewed at head `([0-9a-f]+)`')


def log(msg):
    print(msg, file=sys.stderr)


def load_config(path):
    with open(path) as f:
        cfg = dict(DEFAULTS, **json.load(f))
    if not cfg["github_user"]:
        raise SystemExit("config: github_user must be set")
    cfg["output"] = os.path.expanduser(cfg["output"])
    cfg["token_file"] = os.path.expanduser(cfg["token_file"])
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


def graphql(token, variables, query=QUERY):
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
            log(f"graphql partial errors: {res['errors']}")
        return res["data"]


def fetch_prs(cfg, token):
    prs = {}
    for search in cfg["searches"]:
        q = f"is:pr is:open archived:false sort:updated-desc {search.format(user=cfg['github_user'])}"
        after = None
        while True:
            data = graphql(token, {"q": q, "n": cfg["page_size"], "after": after,
                                   "field": cfg["apreview"]["field"]})
            s = data["search"]
            for node in s["nodes"]:
                if node and node.get("url"):
                    prs[node["url"]] = node
            if not s["pageInfo"]["hasNextPage"]:
                break
            after = s["pageInfo"]["endCursor"]
        if s["issueCount"] > 1000:
            log(f"warning: '{q}' matches {s['issueCount']} PRs, search only returns 1000")
    return list(prs.values())


def fetch_apreview_comments(cfg, token, prs):
    '''bodies of the newest AP-Review comment on each PR, keyed by comment id'''
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


def apreview_status(cfg, pr, bodies):
    ar = cfg["apreview"]
    bot = ar["user"].lower()

    def norm(v):
        return re.sub(r'[\s_-]+', ' ', v).strip().upper()

    proj = None
    for item in pr["projectItems"]["nodes"]:
        owner = ((item["project"] or {}).get("owner") or {}).get("login", "")
        val = item.get("fieldValueByName")
        if item["project"]["number"] == ar["project_number"] and owner.lower() == ar["project_owner"].lower() and val:
            proj = {"status": norm(val["name"]), "at": val["updatedAt"]}

    com = None
    mine = [c for c in pr["comments"]["nodes"] if ((c["author"] or {}).get("login") or "").lower() == bot]
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
    return {
        "status": best["status"],
        "at": best["at"],
        "url": com.get("url") or f"https://github.com/orgs/{ar['project_owner']}/projects/{ar['project_number']}",
        "head_stale": bool(head) and not pr["headRefOid"].startswith(head),
    }


def repo_wanted(cfg, repo):
    if cfg["include_repos"] and not any(fnmatch.fnmatch(repo, p) for p in cfg["include_repos"]):
        return False
    return not any(fnmatch.fnmatch(repo, p) for p in cfg["exclude_repos"])


def summarise(cfg, pr, bodies):
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
    if "review_decision" in reset_on:
        parts["review_decision"] = pr["reviewDecision"] or ""
    if "conflict" in reset_on:
        parts["conflict"] = pr["mergeable"] == "CONFLICTING"
    apreview = apreview_status(cfg, pr, bodies)
    if "apreview" in reset_on:
        parts["apreview"] = apreview["status"]
    if "push" in reset_on:
        parts["push"] = pr["headRefOid"]
    key = hashlib.sha1(json.dumps(parts, sort_keys=True).encode()).hexdigest()[:16]

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
        "mergeable": pr["mergeable"],
        "review": pr["reviewDecision"] or "",
        "changes_by": changes_by,
        "approved_by": approved_by,
        "requested": requested,
        "ncomments": ncomments,
        "events": events[:50],
        "my_last": max(my_times) if my_times else "",
        "apreview": apreview,
        "parts": parts,
        "key": key,
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


def render(cfg, prs):
    data = {
        "generated": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "user": cfg["github_user"],
        "refresh_seconds": cfg["refresh_seconds"],
        "stale_minutes": cfg["stale_minutes"],
        "prs": prs,
    }
    blob = json.dumps(data).replace("</", "<\\/")
    with open(os.path.join(SCRIPT_DIR, "template.html")) as f:
        tpl = f.read()
    title = cfg["title"].replace("&", "&amp;").replace("<", "&lt;")
    return tpl.replace("@@TITLE@@", title).replace("@@DATA@@", blob)


def main():
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("-c", "--config", default=os.path.join(SCRIPT_DIR, "config.json"))
    ap.add_argument("--json", help="also write the PR data to this file")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    cfg = load_config(args.config)

    # skip if the previous cron run is still going
    lock = open(cfg["output"] + ".lock", "w")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        log("previous run still active, skipping")
        return

    token = get_token(cfg)
    raw = fetch_prs(cfg, token)
    raw = [pr for pr in raw if repo_wanted(cfg, pr["repository"]["nameWithOwner"])]
    bodies = fetch_apreview_comments(cfg, token, raw)
    prs = [summarise(cfg, pr, bodies) for pr in raw]
    write_atomic(cfg["output"], render(cfg, prs))
    if args.json:
        write_atomic(args.json, json.dumps(prs, indent=1))
    if args.verbose:
        log(f"{len(prs)} PRs, {len(bodies)} AP-Review comments -> {cfg['output']}")


if __name__ == "__main__":
    main()
