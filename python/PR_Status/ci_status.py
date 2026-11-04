#!/usr/bin/env python3
'''
Generate a static HTML page showing who is holding the GitHub Actions queue of a repo.

Shows running and queued jobs grouped by PR and owner, plus pushes and runs per
owner over the last few hours. Run from cron.
'''

import argparse
import fcntl
import json
import os
import time
import urllib.error
import urllib.parse
import urllib.request
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta, timezone

from pr_status import SCRIPT_DIR, get_token, log, write_atomic

ACTIVE = ("in_progress", "queued", "waiting", "pending", "requested")
# listing runs with filters stops at 1000 results
MAX_RUNS = 1000
# events that mean someone pushed a commit
PUSH_EVENTS = ("push", "pull_request", "pull_request_target")


class API:
    def __init__(self, token, repo):
        self.token = token
        self.base = f"https://api.github.com/repos/{repo}"
        self.remaining = None
        self.warnings = []

    def get(self, path, **params):
        url = f"{self.base}/{path}?{urllib.parse.urlencode(params)}"
        for attempt in range(4):
            req = urllib.request.Request(url, headers={
                "Authorization": f"bearer {self.token}",
                "Accept": "application/vnd.github+json",
                "User-Agent": "ci_status.py",
            })
            try:
                with urllib.request.urlopen(req, timeout=60) as r:
                    self.remaining = r.headers.get("X-RateLimit-Remaining")
                    return json.load(r)
            except urllib.error.HTTPError as e:
                if e.code < 500 or attempt == 3:
                    raise
                log(f"retry {path} after: {e}")
            except (urllib.error.URLError, TimeoutError) as e:
                if attempt == 3:
                    raise
                log(f"retry {path} after: {e}")
            time.sleep(2 * (attempt + 1))

    def warn(self, msg):
        log("warning: " + msg)
        self.warnings.append(msg)

    def paged(self, path, key, first=None, **params):
        '''all pages of a list endpoint. Pages are fetched in turn, as callers
        are already spread over the thread pool'''
        first = first or self.get(path, per_page=100, page=1, **params)
        total = first["total_count"]
        if key == "workflow_runs" and total > MAX_RUNS:
            self.warn(f"{path} {params} has {total} runs, only {MAX_RUNS} are shown")
            total = MAX_RUNS
        items = first[key]
        for p in range(2, (total + 99) // 100 + 1):
            items += self.get(path, per_page=100, page=p, **params)[key]
        return items

    def runs_created(self, start, end):
        '''runs created between start and end, splitting the window to stay under the cap'''
        created = f"{iso(start)}..{iso(end)}"
        first = self.get("actions/runs", per_page=100, page=1, created=created)
        if first["total_count"] > MAX_RUNS and end - start > timedelta(minutes=2):
            mid = start + (end - start) / 2
            return self.runs_created(start, mid) + self.runs_created(mid, end)
        return self.paged("actions/runs", "workflow_runs", first, created=created)


def iso(t):
    return t.strftime("%Y-%m-%dT%H:%M:%SZ")


def fork_owner(run):
    return (((run.get("head_repository") or {}).get("owner")) or {}).get("login") or "?"


def owner(run, repo):
    '''the fork owner for runs from forks, otherwise whoever triggered the run'''
    if (run.get("head_repository") or {}).get("full_name", repo) != repo:
        return fork_owner(run)
    return (run.get("actor") or {}).get("login") or "?"


def fetch(api, repo, hours):
    with ThreadPoolExecutor(24) as ex:
        runs = {}
        for items in ex.map(lambda s: api.paged("actions/runs", "workflow_runs", status=s), ACTIVE):
            runs.update((r["id"], r) for r in items)
        active = list(runs.values())
        jobs = list(ex.map(lambda r: api.paged(f"actions/runs/{r['id']}/jobs", "jobs"), active))

        now = datetime.now(timezone.utc).replace(microsecond=0)
        history = {}
        for items in ex.map(lambda h: api.runs_created(now - timedelta(hours=h + 1), now - timedelta(hours=h)),
                            range(hours)):
            history.update((r["id"], r) for r in items)

        # fork PR runs have no pull_requests, so look the PR up from its head branch
        heads = {(fork_owner(r), r["head_branch"]) for r in active if not own_prs(r, repo)}
        heads = [h for h in heads if h[1]]
        found = ex.map(lambda h: api.get("pulls", head=f"{h[0]}:{h[1]}", state="open"), heads)
        prs = {h: (p[0] if p else None) for h, p in zip(heads, found)}
    return active, jobs, list(history.values()), prs


def own_prs(run, repo):
    '''PRs of a run that are against repo, workflow_run events can list PRs in forks'''
    return [p for p in run["pull_requests"] if p["base"]["repo"]["url"].endswith("/repos/" + repo)]


def pr_for(run, prs, repo):
    '''(number, url, title, owner) of the PR a run is for, or None'''
    if own_prs(run, repo):
        n = own_prs(run, repo)[0]["number"]
        return n, f"https://github.com/{repo}/pull/{n}", run["display_title"], owner(run, repo)
    p = prs.get((fork_owner(run), run["head_branch"]))
    if p:
        return p["number"], p["html_url"], p["title"], p["user"]["login"]
    return None


def summarise(repo, active, jobs, history, prs):
    groups = {}
    totals = {"running": 0, "queued": 0, "labels": defaultdict(lambda: [0, 0]), "oldest_queued": ""}
    for run, jl in zip(active, jobs):
        pr = pr_for(run, prs, repo)
        if pr:
            key = f"#{pr[0]}"
            g = dict(pr=pr[0], url=pr[1], title=pr[2], owner=pr[3], event="pull_request")
        else:
            who = owner(run, repo)
            key = f"{who}:{run['head_branch']}:{run['event']}"
            g = dict(pr=None, url=f"https://github.com/{repo}/actions?query=" +
                     urllib.parse.quote(f"branch:{run['head_branch']} event:{run['event']}"),
                     title=f"{run['head_branch']} ({run['event']})", owner=who, event=run["event"])
        g = groups.setdefault(key, dict(g, running=0, queued=0, runs=0, oldest_queued="",
                                        labels=Counter(), workflows=Counter()))
        g["runs"] += 1
        for j in jl:
            if j["status"] == "completed":
                continue
            label = ",".join(j["labels"]) or "?"
            if j["status"] == "in_progress":
                g["running"] += 1
                totals["running"] += 1
                totals["labels"][label][0] += 1
                g["labels"][label] += 1
                g["workflows"][run["name"]] += 1
            else:
                g["queued"] += 1
                totals["queued"] += 1
                totals["labels"][label][1] += 1
                created = j.get("created_at") or run["created_at"]
                for d in (g, totals):
                    if not d["oldest_queued"] or created < d["oldest_queued"]:
                        d["oldest_queued"] = created
    groups = [g for g in groups.values() if g["running"] or g["queued"]]

    # pushes are distinct pushed head commits; cancelled runs were mostly superseded by a newer push
    def new():
        return dict(runs=0, cancelled=0, shas=set(), events=set(), branches=set(), last="", title="")
    hist = defaultdict(new)
    hist_owners = defaultdict(new)
    for r in history:
        who = owner(r, repo)
        for h in (hist[(who, r["head_branch"])], hist_owners[who]):
            h["runs"] += 1
            h["cancelled"] += r.get("conclusion") == "cancelled"
            h["events"].add(r["event"])
            h["branches"].add(r["head_branch"])
            if r["event"] in PUSH_EVENTS:
                h["shas"].add(r["head_sha"])
            if r["created_at"] > h["last"]:
                h["last"] = r["created_at"]
                h["title"] = r["display_title"]
    branches = []
    for (who, branch), h in hist.items():
        if "pull_request" in h["events"]:
            url = f"https://github.com/{repo}/pulls?q=" + urllib.parse.quote(f"is:pr head:{branch}")
        else:
            url = f"https://github.com/{repo}/actions?query=" + urllib.parse.quote(f"branch:{branch}")
        branches.append(dict(owner=who, branch=branch, events=sorted(h["events"]), title=h["title"],
                             runs=h["runs"], cancelled=h["cancelled"], pushes=len(h["shas"]),
                             last=h["last"], url=url))
    owners = [dict(owner=who, branches=len(h["branches"]), runs=h["runs"], cancelled=h["cancelled"],
                   pushes=len(h["shas"])) for who, h in hist_owners.items()]

    totals["labels"] = dict(totals["labels"])
    return totals, groups, branches, owners


def render(args, warnings, totals, groups, branches, owners):
    data = {
        "generated": iso(datetime.now(timezone.utc)),
        "repo": args.repo,
        "hours": args.hours,
        "refresh_seconds": args.refresh,
        "stale_minutes": args.stale,
        "pr_status": args.pr_status,
        "totals": totals,
        "groups": groups,
        "branches": branches,
        "hist_owners": owners,
        "warnings": warnings,
    }
    # no '<' at all, so nothing in the data can end or confuse the script element
    blob = json.dumps(data).replace("<", "\\u003c")
    with open(os.path.join(SCRIPT_DIR, "ci_template.html")) as f:
        tpl = f.read()
    title = f"{args.repo} CI queue".replace("&", "&amp;").replace("<", "&lt;")
    return tpl.replace("@@TITLE@@", title).replace("@@DATA@@", blob)


def main():
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("--repo", default="ArduPilot/ardupilot")
    ap.add_argument("-o", "--output", default=os.path.join(SCRIPT_DIR, "ci_status.html"))
    ap.add_argument("--hours", type=int, default=24, help="history to show pushes for")
    ap.add_argument("--refresh", type=int, default=600, help="page reload seconds")
    ap.add_argument("--stale", type=int, default=30, help="minutes before the page shows as stale")
    ap.add_argument("--token-file", default="", help="GitHub token file, else $GH_TOKEN or gh auth token")
    ap.add_argument("--pr-status", default="", help="URL of a PR status page to link to")
    ap.add_argument("--json", help="also write the data to this file")
    ap.add_argument("--lock", help="lock file to stop overlapping runs (default: OUTPUT.lock)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    lockfile = args.lock or args.output + ".lock"
    os.makedirs(os.path.dirname(os.path.abspath(lockfile)), exist_ok=True)
    lock = open(lockfile, "w")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        log("previous run still active, skipping")
        return

    t0 = time.monotonic()
    api = API(get_token({"token_file": os.path.expanduser(args.token_file)}), args.repo)
    active, jobs, history, prs = fetch(api, args.repo, args.hours)
    totals, groups, branches, owners = summarise(args.repo, active, jobs, history, prs)
    write_atomic(args.output, render(args, api.warnings, totals, groups, branches, owners))
    if args.json:
        write_atomic(args.json, json.dumps(dict(totals=totals, groups=groups, branches=branches,
                                                hist_owners=owners, warnings=api.warnings), indent=1))
    if args.verbose:
        log(f"{totals['running']} running, {totals['queued']} queued jobs in {len(active)} runs, "
            f"{len(history)} runs in {args.hours}h, {time.monotonic() - t0:.1f}s, "
            f"rate limit left {api.remaining} -> {args.output}")


if __name__ == "__main__":
    main()
