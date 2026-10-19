# PR_Status

Builds a static HTML page showing the status of your open GitHub PRs. It needs Python 3.8+ (standard library only) on Linux or macOS, and a GitHub token (see below).

## Install

    # copy pr_status.py, template.html and config.example.json into one directory
    ./pr_status.py --init              # writes ~/.config/pr_status/config.json for your token's login
    ./pr_status.py -v                  # first run
    xdg-open ~/.config/pr_status/pr_status.html

Then add the cron line that `--init` prints, e.g.

    */15 * * * * /usr/bin/python3 /path/to/pr_status.py

Options: `-c CONFIG`, `--init`, `-v`, `--json FILE` to dump the data.

The config is the first that exists of: `-c`, `$PR_STATUS_CONFIG`, `$XDG_CONFIG_HOME/pr_status/config.json` (default `~/.config/...`), then `config.json` next to the script. Relative `output` and `token_file` paths are relative to the config file. `config.example.json` lists every key.

The script never changes the existing page if a fetch fails. It exits non-zero, so cron mails you the error.

## Token

The script tries these in order: `$GH_TOKEN` / `$GITHUB_TOKEN`, then `token_file`, then `gh auth token`.

`gh auth token` uses the desktop keyring, so it only works while you are logged in, and cron's `PATH` may not include `gh`. To run headless, put a token in a file with `chmod 600` and set `token_file`. A classic token with the `repo` scope (or `public_repo` for public repos only) is enough. A project-based `apreview` also needs `read:project`.

## Page

- **✓** marks a PR as dealt with. Ticks are stored in the browser's localStorage along with a fingerprint of the PR. When the fingerprint changes, the tick clears and the row gets an orange bar saying why.
- **New** counts comments and reviews by others since you ticked the PR. For a PR you have never ticked, it counts since your own last comment, review or commit.
- Closed and merged PRs disappear on the next run, and their ticks are pruned.
- Click a column header to sort it. Type to search; all words must match. `/` focuses the search box and Esc clears it.
- The page reloads itself every `refresh_seconds`. The "updated" time turns red after `stale_minutes`.

## config.json

| key | meaning |
|---|---|
| `github_user` | your login. "Others" means anyone but you. If empty, it is the token's login. |
| `searches` | GitHub search qualifiers, one search each, merged. `{user}` is replaced by `github_user`. e.g. `"review-requested:{user}"` |
| `include_repos` | fnmatch patterns such as `"ArduPilot/*"`. An empty list means all repos. |
| `exclude_repos` | fnmatch patterns to drop. |
| `ignore_users` | logins whose comments don't count as activity. |
| `ignore_bots` | ignore GitHub App/bot accounts. |
| `reset_on` | what clears a tick: `activity` (comment or review by others), `ci_failure` (CI enters or leaves failing), `ci` (any CI state change), `review_decision`, `conflict`, `apreview` (APReview verdict), `push` (new head commit) |
| `apreview` | optional; `null` hides the APReview column. Where the verdict comes from: the `field` single-select (default `Result`) on org project `project_owner`/`project_number`, and/or the `**Verdict: X**` line in the newest comment by `user`. When both exist, the newer one wins. ArduPilot uses `{"user": "AP-Review", "project_owner": "ArduPilot", "project_number": 33, "field": "Result"}`. |
| `output` | where the HTML page is written (default `pr_status.html` beside the config). |
| `title` | page title (default "`<user>` PR Status"). |
| `refresh_seconds`, `stale_minutes`, `token_file`, `page_size` | as named. If GitHub returns 502s, lower `page_size`. |
