# Review PRs by Label

Review all GitHub PRs with the specified label and generate an HTML report. Checks the main ArduPilot repo, the ArduPilot wiki repo, and all ArduPilot-owned submodule repos.

Run this from the root of an ArduPilot checkout (it reads `.gitmodules` in the working directory). The report is written to the repository root and works in any ArduPilot checkout, not just one.

## Arguments
- `$ARGUMENTS` - The GitHub label to filter PRs (e.g., DevCallTopic, Copter, Plane)

## Allowed Tools
- Bash(gh pr list *)
- Bash(gh pr diff *)
- Bash(gh pr checks *)
- Bash(gh pr view *)
- Bash(gh api *)
- Bash(git log *)
- Bash(git diff *)
- Bash(git show *)
- Bash(git config *)
- Bash(git rev-parse *)
- Bash(codex-session:*)
- Bash(codex:*)
- Bash(codex)
- Bash(rsync:*)

## Task

The report file is `devcall_pr_reviews.html` in the repository root — i.e. `$(git rev-parse --show-toplevel)/devcall_pr_reviews.html`. Use this path everywhere below (when run in the main ArduPilot checkout this resolves to e.g. `/data/APM.claude/devcall_pr_reviews.html`).

1. Find all PRs with the label "$ARGUMENTS" across the main repo, the wiki repo, and submodules:
   - Main repo: `gh pr list --label "$ARGUMENTS" --json number,title,author,url,updatedAt --limit 50`
   - Wiki repo: `gh pr list --repo ArduPilot/ardupilot_wiki --label "$ARGUMENTS" --json number,title,author,url,updatedAt --limit 50`
   - Parse `.gitmodules` to find all submodule URLs hosted under `ArduPilot/` or `ardupilot/` on GitHub
   - For each ArduPilot-owned submodule repo, run: `gh pr list --repo <owner/repo> --label "$ARGUMENTS" --json number,title,author,url,updatedAt --limit 50`
   - Combine all results, tracking which repo each PR belongs to. Note that wiki PRs are documentation-focused (ReST under `*/source/docs/`); review for technical accuracy vs the current ArduPilot codebase, broken `:ref:` cross-references, ReST syntax, and consistency with existing wiki conventions.

2. For each PR found (in any repo):
   - Fetch the diff with `gh pr diff <number>` (add `--repo <owner/repo>` for submodule PRs)
   - Check CI status with `gh pr checks <number>` (add `--repo <owner/repo>` for submodule PRs)
   - Review the code changes for:
     - Bugs or logic errors
     - Code style issues (trailing whitespace, inconsistent indentation)
     - Missing error handling
     - Memory concerns for embedded targets
     - Alignment with ArduPilot coding patterns

3. Write the HTML report directly to the report file (`devcall_pr_reviews.html` in the repository root) — do not read the file first, it is pure generated output:
   - Table of contents with author name next to each PR number and quick verdict
   - For submodule and wiki PRs, prefix the PR entry with the repo name (e.g., "[ChibiOS] #123", "[wiki] #7730")
   - For each PR:
     - Link to the PR on GitHub (using the correct repo URL)
     - Author name
     - Repo name (if not the main ArduPilot repo)
     - CI status (passing/failing)
     - List of files changed with links to GitHub diff view
     - Review findings with specific file:line references linking to GitHub
     - Overall verdict: APPROVE, COMMENT, or REQUEST CHANGES
   - Include the absolute review date at the top of the report.
   - Summary table at the end

4. Verdicts should be:
   - **APPROVE**: Code is correct and ready to merge
   - **COMMENT**: Minor issues that should be noted but don't block merge
   - **REQUEST CHANGES**: Bugs or significant issues that must be fixed

5. For each finding, include a direct link to the relevant line in GitHub's PR diff view using the format:
   `https://github.com/<owner>/<repo>/pull/<number>/files#diff-<filepath>R<line>`

6. **Validate the findings with Codex, then revise the report based on the result.** After the HTML report is written, run the OpenAI Codex CLI as an independent second reviewer to cross-check the findings before finalising. This is a required step, not optional.

   - Invoke Codex non-interactively (same mechanism as the `/codex` command — `codex-session` threads context per working directory and reads from `/dev/null` so it never blocks on stdin):
     ```bash
     codex-session "$CODEX_TASK" </dev/null
     ```
     Capture Codex's full stdout — it runs agentically against the live repo and may take a while. Set `$CODEX_TASK` to instruct Codex to:
     - Read the generated report (`devcall_pr_reviews.html` in the repository root).
     - For each finding, independently fetch the relevant PR diff (`gh pr diff <number> [--repo <owner/repo>]`, plus `gh pr checks`/`gh pr view` as needed) and verify: (a) the issue is real, (b) the `file:line` reference is correct and still present at the PR's current head, (c) the severity and the PR's overall verdict (APPROVE / COMMENT / REQUEST CHANGES) are appropriate.
     - Explicitly call out **false positives** (findings that don't hold up), **mislocated** `file:line` references, **wrong verdicts**, and any **significant issues the report missed**.
     - Output a concise per-PR, per-finding assessment using **CONFIRM / REFUTE / ADJUST** (with the corrected detail for ADJUST), plus any **NEW** findings it discovered.

   - Then **evaluate Codex's assessment with your own judgement — do not blindly accept it.** Codex is a second opinion, not an authority: re-check anything it disputes against the actual diff before acting. Treat agreement between you and Codex as higher-confidence, and treat any contradiction with claims made earlier in the session as something to flag, not silently resolve.

   - **Modify the report based on that evaluation:** remove or correct findings Codex refuted (and you agree are wrong), fix mislocated `file:line` references, adjust severities and any affected PR verdicts, and add any validated NEW findings. Where you disagree with Codex after re-checking, keep your finding but add a one-line note of the disagreement and why.

   - Record the validation in the report itself: add a short **"Codex validation"** line near the top (date + one-line outcome, e.g. "Codex cross-checked N findings: X confirmed, Y adjusted, Z refuted, W added"). If any verdicts changed, update the per-PR sections, the contents/quick-verdict table, the summary table, and the final totals so the whole report stays consistent.

7. **Publish the report to the web.** Once the report is finalised (including the Codex revisions), publish it to a dated public directory so it has a shareable URL. This is automatic — always do it at the end of the run.

   ```bash
   REPORT="$(git rev-parse --show-toplevel)/devcall_pr_reviews.html"
   DATE=$(date +%Y_%m_%d)   # e.g. 2026_06_09 — must match the report's review date
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$DATE/
   ```

   - This serves the report at `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html`.
   - `--mkpath` creates the dated directory if it doesn't exist (the `UAV-web/DevCallReviews/` parent already exists). If a host's rsync predates 3.2.3 and lacks `--mkpath`, first run `ssh tridgell.net mkdir -p UAV-web/DevCallReviews/$DATE`, then rsync without `--mkpath`.
   - Use the SAME `<DATE>` as the report's review date so the directory date and the in-report date agree.
   - Print the resulting public URL when done.

Report the summary counts when complete, note the validation outcome, and give the published URL: X APPROVE | Y COMMENT | Z REQUEST CHANGES (Codex: X confirmed / Y adjusted / Z refuted / W added) — published at https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html
