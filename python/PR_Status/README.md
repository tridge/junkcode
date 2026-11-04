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

GitHub's search sometimes leaves PRs out without reporting an error. The script remembers the PRs on the last page (in `.<page name>.known.json` next to the page), and looks up any that search missed directly, so they don't drop off the page.

The script never changes the existing page if a fetch fails (the `devcall` lookup is the one exception, see below). It exits non-zero, so cron mails you the error.

## Token

The script tries these in order: `$GH_TOKEN` / `$GITHUB_TOKEN`, then `token_file`, then `gh auth token`.

`gh auth token` uses the desktop keyring, so it only works while you are logged in, and cron's `PATH` may not include `gh`. To run headless, put a token in a file with `chmod 600` and set `token_file`. A classic token with the `repo` scope (or `public_repo` for public repos only) is enough. A project-based `apreview` also needs `read:project`.

## Page

- **✓** marks a PR as dealt with. Ticks are stored in the browser's localStorage along with a fingerprint of the PR. When the fingerprint changes, the tick clears and the row gets an orange bar saying why.
- **New** counts comments and reviews by others since you ticked the PR. For a PR you have never ticked, it counts since your own last comment, review or commit.
- Closed and merged PRs disappear on the next run. Their ticks are pruned once the PR has been missing for a week, so a run that misses PRs by mistake doesn't lose ticks.
- For a PR from another repo, such as a fork, where CI ran on the branch, the CI column also shows a smaller "fork:" badge for the CI that ran on the branch there, e.g. on push. It links to that repo's Actions runs for the branch. The column sorts on the PR's own CI.
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
| `reset_on` | what clears a tick: `activity` (comment or review by others), `ci_failure` (CI enters or leaves failing), `ci` (any CI state change), `branch_ci_failure` (CI on the PR's branch in its own repo, e.g. your fork, enters or leaves failing), `branch_ci` (any change in that CI), `review_decision`, `conflict` (ignored while GitHub reports the merge state as unknown), `apreview` (APReview verdict), `push` (new head commit), `devcall` (enters or leaves the `devcall` filter) |
| `apreview` | optional; `null` hides the APReview column. Where the verdict comes from: the `field` single-select (default `Result`) on org project `project_owner`/`project_number`, and/or the `**Verdict: X**` line in the newest comment by `user`. When both exist, the newer one wins. ArduPilot uses `{"user": "AP-Review", "project_owner": "ArduPilot", "project_number": 33, "field": "Result"}`. |
| `devcall` | adds a `label` tag, in front of the GitHub labels, to PRs that match `filter` (project board filter syntax, e.g. `"status:ReadyForDevCall is:open"`) on the public org project `project_owner`/`project_number`. It is read with an anonymous REST request, which needs no token. If the request fails, the page is still written without the tags, and the header says so. Leave it empty to turn it off. |
| `output` | where the HTML page is written (default `pr_status.html` beside the config). |
| `title` | page title (default "`<user>` PR Status"). |
| `refresh_seconds`, `stale_minutes`, `token_file`, `page_size` | as named. If GitHub returns 502s, lower `page_size`. |
| `ci_status` | optional URL of a `ci_status.py` page to link to from the header. |

## CI queue page

`ci_status.py` writes `ci_status.html`, a page showing who is holding a repo's GitHub Actions queue. It is not specific to you, so it can be hosted for everyone; ArduPilot's is at https://firmware.ardupilot.org/Tools/CIJobs/. It shows:

- running and queued jobs, in total and per runner label, and the age of the oldest queued job
- running and queued jobs per owner, and per PR or branch
- pushes, workflow runs and cancelled runs per owner and per branch over the last `--hours` (default 24)

A run makes about 200 API calls and takes 20-30s, so run it every 10-15 minutes, e.g.

    */10 * * * * /usr/bin/python3 /path/to/ci_status.py --pr-status pr_status.html

Options: `--repo` (default `ArduPilot/ardupilot`), `-o OUTPUT`, `--hours`, `--refresh`, `--stale`, `--token-file`, `--pr-status URL` to link back to a PR status page, `--json FILE`, `--lock FILE` to keep the lock out of a web directory, `-v`. It finds a token the same way as `pr_status.py` and needs `pr_status.py` and `ci_template.html` beside it.
