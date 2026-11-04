# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

PR_Status is one small tool inside a large, unrelated junkcode repo. Work only in this directory. README.md is the user-facing documentation: it covers config keys, token lookup, and page behaviour. Keep it in sync when behaviour changes.

## Running

```
./pr_status.py -v                         # config from -c, $PR_STATUS_CONFIG, ~/.config/pr_status/config.json, or config.json next to the script
./pr_status.py -c other.json --json /tmp/prs.json   # also dump the summarised PR data
./pr_status.py --init                     # write a starter config from config.example.json
flake8 pr_status.py
```

There are no tests and no dependencies outside the standard library (it uses `urllib` for GitHub GraphQL, not `requests` or `gh`). Each run makes real GitHub API calls and needs a token: `$GH_TOKEN`/`$GITHUB_TOKEN`, then `token_file`, then `gh auth token`. To check a change, run it with `--json` and open the generated `pr_status.html` in a browser.

`config.json` is personal and gitignored. `config.example.json` is the tracked template that `--init` copies, so new config keys belong there with an off-by-default value. Relative `output` and `token_file` paths are relative to the config file.

## Architecture

There are two halves, joined by one JSON blob:

1. **`pr_status.py`** (runs under cron) runs one GraphQL `search` per entry in `searches`, merges the results by URL, adds back any PRs from the last run that search missed (see below), and filters them by repo fnmatch. It then makes a second batched `nodes(ids:)` query to fetch the bodies of the newest AP-Review bot comments. `summarise()` reduces each PR to a flat dict. `render()` replaces `@@TITLE@@` and `@@DATA@@` in `template.html`, escaping every `<` in the JSON, and `write_atomic()` swaps the file into place. A `flock` on `<output>.lock` skips the run if the previous one is still going. Any failure raises before the write, so the old page stays up. The devcall lookup is the exception.
2. **`template.html`** is a self-contained page with inline CSS and JS and no build step. It reads the blob from `<script id="prdata">`, then does all sorting, searching, tick state and "New" counting client-side. The state lives in `localStorage` (`prstatus.ticks.v1`, `prstatus.prefs.v1`).

**Tick and reset mechanism.** This contract spans both files. For each `reset_on` option, `summarise()` adds a value to a `parts` dict. When the user ticks a PR, the page stores `{parts, seen}`, plus `gone` while the PR is missing from the data. On later loads, `tickState()` compares the parts one by one: any difference clears the tick, and the differing keys become the reasons shown (the `WHY` map in the template). A `null` part means the value couldn't be fetched, so it neither keeps nor clears a tick. Adding a new `reset_on` option therefore takes three changes: a branch in `summarise()`, a `WHY` entry in the template, and a row in the README table. Turning on a new option clears existing ticks, because their stored parts don't have it yet.

**APReview is optional.** With `apreview` null, the column is hidden and `apreview` is null in each PR. The project fields need the `read:project` scope, so `pr_query()` only fills the `@@FIELDVAR@@` and `@@PROJECT@@` placeholders in `QUERY` and `PR_NODES_QUERY` when an APReview project is configured. Any new query that uses the `pr` fragment must go through `pr_query()`.

**APReview verdict** comes from two sources: a single-select field on an org ProjectV2 (`projectItems.fieldValueByName`), and a `**Verdict: X**` line in the bot's newest comment. Whichever is newer wins. The comment still supplies the report URL and the `Reviewed at head` SHA, which is used to flag a stale review. Status strings are normalised to uppercase with spaces, and the template's `APR` map keys on those strings.

**Dev-call tag** (`devcall` config, shown in front of the PR labels) is a separate anonymous REST call to `/orgs/{owner}/projectsV2/{n}/items?q=<filter>`. It's anonymous because the project is public and the user's token can't see ArduPilot projects. Unlike every other fetch, a failure here isn't fatal: the PRs' `devcall` values become `null`, and the page shows "status unavailable" in the header.

**`ci_status.py`** is a separate page showing who holds a repo's GitHub Actions queue, rendered from `ci_template.html`. It imports `SCRIPT_DIR`, `get_token`, `log` and `write_atomic` from `pr_status.py`, so keep those signatures stable.

**GitHub quirks handled in code:** search with nested connections returns occasional 502s, so `graphql()` retries, and users can lower `page_size`. Partial GraphQL errors are logged but not treated as fatal. When the token can't read a project, GitHub returns `null` in `projectItems` and reports no error. This happens with fine-grained PATs that lack org Projects access. Treat every node in a connection as possibly null. `mergeable` is `UNKNOWN` while GitHub recomputes, often for most PRs at once after the base branch moves, so the `conflict` part is `null` then. Search sometimes returns an arbitrary subset, even nothing, and its `issueCount` shrinks to match, so retrying on a short count doesn't help. Search is therefore used only to discover PRs. The URLs and node IDs of the PRs on the last page are saved in `.<output name>.known.json`. Any PR search missed is fetched directly with `PR_NODES_QUERY` (sharing the `PR_FIELDS` fragment with the search) and kept if it's still open, for up to `KEEP_SECONDS` after search last found it. The page also waits a week before pruning a missing PR's tick. Search caps out at 1000 results, so the script warns past that. Connection sizes such as `comments(last: 30)` and `reviews(last: 30)` limit how far back the activity and "New" counts can see.
