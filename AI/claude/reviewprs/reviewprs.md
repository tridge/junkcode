# Review PRs by Label

Review all GitHub PRs with the specified label and generate an HTML report. Checks the main ArduPilot repo, the ArduPilot wiki repo, and all ArduPilot-owned submodule repos.

Run this from the root of an ArduPilot checkout (it reads `.gitmodules` in the working directory). The report is written to the repository root and works in any ArduPilot checkout, not just one.

The command is incremental: it records the head commit hash each PR was reviewed at, and on a re-run it reuses the previously-published report for any PR whose head is unchanged, only re-reviewing PRs that are new or have changed. This makes a re-run to pick up new/updated PRs very fast.

## Arguments
- `$ARGUMENTS` - The GitHub label to filter PRs (e.g., DevCallTopic, Copter, Plane)

## Allowed Tools
- Bash(gh pr list *)
- Bash(gh pr diff *)
- Bash(gh pr checks *)
- Bash(gh pr view *)
- Bash(gh pr comment *)
- Bash(gh api *)
- Bash(git log *)
- Bash(git diff *)
- Bash(git show *)
- Bash(git config *)
- Bash(git rev-parse *)
- Bash(curl:*)
- Bash(codex-session:*)
- Bash(codex:*)
- Bash(codex)
- Bash(rsync:*)

## Task

The report file is `devcall_pr_reviews.html` in the repository root — i.e. `$(git rev-parse --show-toplevel)/devcall_pr_reviews.html`. Use this path everywhere below (when run in the main ArduPilot checkout this resolves to e.g. `/data/APM.claude/devcall_pr_reviews.html`).

The label is `$ARGUMENTS`. Two published locations matter (see step 8):
- per-label "latest": `https://uav.tridgell.net/DevCallReviews/$ARGUMENTS/devcall_pr_reviews.html` — the canonical current report for this label, and the one a re-run reads to decide what to skip.
- dated archive: `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html`.

1. Find all PRs with the label "$ARGUMENTS" across the main repo, the wiki repo, and submodules. **Capture each PR's current head commit** (`headRefOid`); the short hash is its first 10 characters.
   - Main repo: `gh pr list --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Wiki repo: `gh pr list --repo ArduPilot/ardupilot_wiki --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Parse `.gitmodules` to find all submodule URLs hosted under `ArduPilot/` or `ardupilot/` on GitHub
   - For each ArduPilot-owned submodule repo, run: `gh pr list --repo <owner/repo> --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Combine all results, tracking which repo each PR belongs to. Give each PR a stable **key**: the PR number for the main repo, or `<reponame>#<number>` for wiki/submodule PRs (e.g. `mavlink#360`, `wiki#7730`). Note that wiki PRs are documentation-focused (ReST under `*/source/docs/`); review for technical accuracy vs the current ArduPilot codebase, broken `:ref:` cross-references, ReST syntax, and consistency with existing wiki conventions.

2. **Fast incremental skip — decide what actually needs reviewing.** Fetch the previously-published report for this label and compare head hashes:
   - `curl -fsS https://uav.tridgell.net/DevCallReviews/$ARGUMENTS/devcall_pr_reviews.html` (a 404/failure means this is the first run for this label — treat every PR as new and review all of them).
   - Parse its review manifest, an HTML comment of the form:
     `<!-- reviewprs-manifest v1 label="..." generated="YYYY-MM-DD" heads="<key>:<shorthash> <key>:<shorthash> ..." -->`
     giving the head each PR was last reviewed at.
   - Classify every PR found in step 1:
     - **REUSE** — key is in the manifest AND its current short head == the manifest's head. Already reviewed at this exact commit; do NOT re-review. Carry its content over verbatim in step 4.
     - **REVIEW** — key absent from the manifest (new PR) OR current head differs (changed PR). These are the only PRs that get a diff-fetch + review.
     - **DROPPED** — key in the manifest but no longer in the current labelled set (merged, closed, or label removed). Do not carry it into the open-PR sections/totals; if useful, check `gh pr view <n> --json state,mergedAt` and note it as merged/closed.
   - Report the split before proceeding, e.g. "12 labelled PRs: 2 to review (1 new, 1 changed), 10 reused, 1 dropped (merged)".
   - **Refresh CI for the REUSE set (cheap).** A reused PR's code is unchanged, but its CI result can still have changed (a flaky job re-run, or its merge-with-master base moved). For each REUSE PR run `gh pr checks <number> [--repo <owner/repo>]` (status only — do NOT re-fetch the diff or re-review). Note which reused PRs have a CI status differing from what their carried-over section currently shows.



3. **Review only the REVIEW set.** For each such PR (in any repo):
   - Fetch the diff with `gh pr diff <number>` (add `--repo <owner/repo>` for submodule PRs)
   - Check CI status with `gh pr checks <number>` (add `--repo <owner/repo>` for submodule PRs)
   - Review the code changes for:
     - Bugs or logic errors
     - Code style issues (trailing whitespace, inconsistent indentation)
     - Missing error handling
     - Memory concerns for embedded targets
     - Alignment with ArduPilot coding patterns

4. Write the HTML report directly to the report file (`devcall_pr_reviews.html` in the repository root) — do not read the file first, it is pure generated output. Build it from the REUSE sections (copied verbatim from the fetched prior report) plus freshly-written sections for the REVIEW set:
   - As the first line inside `<body>`, emit the review manifest comment covering **every** PR in the final report (reused + reviewed): `<!-- reviewprs-manifest v1 label="$ARGUMENTS" generated="<DATE>" heads="<key>:<shorthash> ..." -->`. The next run depends on this.
   - Table of contents with author name next to each PR number and quick verdict
   - For submodule and wiki PRs, prefix the PR entry with the repo name (e.g., "[ChibiOS] #123", "[wiki] #7730")
   - For each PR:
     - Link to the PR on GitHub (using the correct repo URL)
     - Author name
     - Repo name (if not the main ArduPilot repo)
     - The **reviewed head short hash** (e.g. `Head: 69bff866c7`) in the PR's meta line — visible, and matching the manifest
     - CI status (passing/failing)
     - List of files changed with links to GitHub diff view
     - Review findings with specific file:line references linking to GitHub
     - Overall verdict: APPROVE, COMMENT, or REQUEST CHANGES
   - Reused sections are copied unchanged (they were reviewed and Codex-validated in a prior run at the same head) **except for the CI status, which is refreshed** from the step-2 `gh pr checks` result: update the PR's CI indicator in its meta line and in the contents/quick-verdict and summary tables to the current value. If a reused PR's CI flipped, add a brief `CI updated <DATE>: <old> → <new>` note to its section; and if it went green→failing on otherwise-unchanged code, flag it (likely a flaky job or a base-merge regression rather than a fault in the PR diff) so it isn't silently presented as still-passing. Do not change the findings or verdict of a reused PR — only its CI status. Only newly reviewed/changed PRs get fresh content. Add a short note near the top summarising the refresh (what was added / changed / dropped, what was reused, and any CI changes on reused PRs).
   - Include the absolute review date at the top of the report.
   - Summary table at the end

5. Verdicts should be:
   - **APPROVE**: Code is correct and ready to merge
   - **COMMENT**: Minor issues that should be noted but don't block merge
   - **REQUEST CHANGES**: Bugs or significant issues that must be fixed

6. For each finding, include a direct link to the relevant line in GitHub's PR diff view using the format:
   `https://github.com/<owner>/<repo>/pull/<number>/files#diff-<filepath>R<line>`

7. **Validate the findings with Codex, then revise the report based on the result.** After the HTML report is written, run the OpenAI Codex CLI as an independent second reviewer to cross-check the findings before finalising. This is a required step, not optional. **Only validate the PRs reviewed this run (the REVIEW set)** — reused sections were already validated in the prior run, so there is no need to re-validate them.

   - Invoke Codex non-interactively (same mechanism as the `/codex` command — `codex-session` threads context per working directory and reads from `/dev/null` so it never blocks on stdin):
     ```bash
     codex-session "$CODEX_TASK" </dev/null
     ```
     Capture Codex's full stdout — it runs agentically against the live repo and may take a while. Set `$CODEX_TASK` to instruct Codex to:
     - Read the generated report (`devcall_pr_reviews.html` in the repository root).
     - For each finding on the reviewed PRs, independently fetch the relevant PR diff (`gh pr diff <number> [--repo <owner/repo>]`, plus `gh pr checks`/`gh pr view` as needed) and verify: (a) the issue is real, (b) the `file:line` reference is correct and still present at the PR's current head, (c) the severity and the PR's overall verdict (APPROVE / COMMENT / REQUEST CHANGES) are appropriate.
     - Explicitly call out **false positives** (findings that don't hold up), **mislocated** `file:line` references, **wrong verdicts**, and any **significant issues the report missed**.
     - Output a concise per-PR, per-finding assessment using **CONFIRM / REFUTE / ADJUST** (with the corrected detail for ADJUST), plus any **NEW** findings it discovered.

   - Then **evaluate Codex's assessment with your own judgement — do not blindly accept it.** Codex is a second opinion, not an authority: re-check anything it disputes against the actual diff before acting. Treat agreement between you and Codex as higher-confidence, and treat any contradiction with claims made earlier in the session as something to flag, not silently resolve.

   - **Modify the report based on that evaluation:** remove or correct findings Codex refuted (and you agree are wrong), fix mislocated `file:line` references, adjust severities and any affected PR verdicts, and add any validated NEW findings. Where you disagree with Codex after re-checking, keep your finding but add a one-line note of the disagreement and why.

   - Record the validation in the report itself: add a short **"Codex validation"** line near the top (date + one-line outcome, e.g. "Codex cross-checked N findings: X confirmed, Y adjusted, Z refuted, W added"). If any verdicts changed, update the per-PR sections, the contents/quick-verdict table, the summary table, and the final totals so the whole report stays consistent.

8. **Post review comments to the PRs — DevCallEU only.** This step runs automatically **only when the label (`$ARGUMENTS`) is exactly `DevCallEU`**. For every other label, do **not** post any comments unless the user explicitly asks you to in their message. (When they do ask for another label, follow the same mechanics below.)

   Posting happens **after** the report is finalised, i.e. only once a PR has been reviewed by **both you and Codex** (steps 3 and 7). Therefore comments are posted for the **REVIEW set** (the PRs reviewed this run) — reused PRs were already commented in the prior run at the same head, so skip them.

   - **Scope:** post a comment for each reviewed PR whose verdict is **COMMENT** or **REQUEST CHANGES** (i.e. there is something actionable). Skip clean **APPROVE** PRs with only NOTE-level items to avoid noise (post one only if the user asked for it).
   - **Content:** mirror that PR's findings from the finalised report — verdict, then findings grouped by severity, each with its `file:line` reference and a one-line description, plus suggested fixes where useful. Use the post-Codex findings (refuted ones removed, line refs corrected). If you dropped a finding as a false positive during validation, you may note that briefly so the author isn't left chasing it.
   - **Mark every comment as AI-generated.** Begin the body with a marker line, e.g.: `**Automated review note — AI-generated (Claude), validated against the live diff.** Please sanity-check before acting.`
   - **Update, don't duplicate.** If you have already posted an AI-generated comment to that PR (e.g. in a previous run), **edit the existing comment instead of adding a new one**: `gh pr comment <number> [--repo <owner/repo>] --edit-last --body-file <file>` edits your most recent comment on that PR. If `--edit-last` is not viable, find your prior comment via `gh api repos/<owner>/<repo>/issues/<number>/comments` (look for the "AI-generated (Claude)" marker) and PATCH it by id. Only post a brand-new comment (`gh pr comment <number> [--repo <owner/repo>] --body-file <file>`) if you have not commented on that PR before.
   - Add `--repo <owner/repo>` for wiki/submodule PRs.
   - Record in the report (near the top, alongside the Codex line) which PRs got a comment posted/updated, with links to the comments.

9. **Publish the report to the web.** Once the report is finalised (including the Codex revisions and any DevCallEU comment posting), publish it to BOTH the per-label "latest" directory (which the next re-run reads) and a dated archive directory. This is automatic — always do it at the end of the run.

   ```bash
   REPORT="$(git rev-parse --show-toplevel)/devcall_pr_reviews.html"
   LABEL="$ARGUMENTS"
   DATE=$(date +%Y_%m_%d)   # e.g. 2026_06_09 — must match the report's review date
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$LABEL/   # per-label latest
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$DATE/    # dated archive
   ```

   - Latest serves at `https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html`; the dated snapshot at `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html`.
   - `--mkpath` creates the destination directory if it doesn't exist (the `UAV-web/DevCallReviews/` parent already exists). If a host's rsync predates 3.2.3 and lacks `--mkpath`, first `ssh tridgell.net mkdir -p UAV-web/DevCallReviews/<dir>`, then rsync without `--mkpath`.
   - Use the SAME `<DATE>` as the report's review date so the directory date and the in-report date agree.
   - Print both public URLs when done.

Report the summary counts when complete, note the skip split and validation outcome, any PR comments posted/updated (DevCallEU only), and give the published URLs: X APPROVE | Y COMMENT | Z REQUEST CHANGES (reused N, reviewed M; Codex: X confirmed / Y adjusted / Z refuted / W added; comments posted on K PRs) — published at https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html
