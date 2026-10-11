# PR_Status

Builds a static HTML page showing the status of your open GitHub PRs. Run it from cron:

    */15 * * * * /usr/bin/python3 $HOME/junkcode/python/PR_Status/pr_status.py

Options: `-c CONFIG` (default `config.json` next to the script), `-v`, `--json FILE` to dump the data.

The script never changes the existing page if a fetch fails. It exits non-zero, so cron mails you the error.

## Token

The script tries these in order: `$GH_TOKEN` / `$GITHUB_TOKEN`, then `token_file`, then `gh auth token`.

`gh auth token` uses the desktop keyring, so it only works while you are logged in. To run headless, put a token in a file with `chmod 600` and set `token_file`. The token needs the `read:project` scope for the APReview column.

## Page

- **✓** marks a PR as dealt with. Ticks are stored in the browser's localStorage along with a fingerprint of the PR. When the fingerprint changes, the tick clears and the row gets an orange bar saying why.
- **New** counts comments and reviews by others since you ticked the PR. For a PR you have never ticked, it counts since your own last comment, review or commit.
- Closed and merged PRs disappear on the next run, and their ticks are pruned.
- Click a column header to sort it. Type to search; all words must match. `/` focuses the search box and Esc clears it.
- The page reloads itself every `refresh_seconds`. The "updated" time turns red after `stale_minutes`.

## config.json

| key | meaning |
|---|---|
| `github_user` | your login. "Others" means anyone but you. |
| `searches` | GitHub search qualifiers, one search each, merged. `{user}` is replaced by `github_user`. e.g. `"review-requested:{user}"` |
| `include_repos` | fnmatch patterns such as `"ArduPilot/*"`. An empty list means all repos. |
| `exclude_repos` | fnmatch patterns to drop. |
| `ignore_users` | logins whose comments don't count as activity. |
| `ignore_bots` | ignore GitHub App/bot accounts. |
| `reset_on` | what clears a tick: `activity` (comment or review by others), `ci_failure` (CI enters or leaves failing), `ci` (any CI state change), `review_decision`, `conflict`, `apreview` (APReview verdict), `push` (new head commit) |
| `apreview` | where the APReview verdict comes from: the `field` single-select on org project `project_owner`/`project_number`, or the `**Verdict: X**` line in the newest comment by `user`. When both exist, the newer one wins. |
| `output` | where the HTML page is written. |
| `title`, `refresh_seconds`, `stale_minutes`, `token_file`, `page_size` | as named. If GitHub returns 502s, lower `page_size`. |
