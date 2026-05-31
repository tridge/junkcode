# Review PRs by Label

Review all GitHub PRs with the specified label and generate an HTML report. Checks the main ArduPilot repo, the ArduPilot wiki repo, and all ArduPilot-owned submodule repos.

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

## Task

1. Find all PRs with the label "$ARGUMENTS" across the main repo, the wiki repo, and submodules:
   - Main repo: `gh pr list --label "$ARGUMENTS" --json number,title,author,url,updatedAt --limit 50`
   - Wiki repo: `gh pr list --repo ArduPilot/ardupilot_wiki --label "$ARGUMENTS" --json number,title,author,url,updatedAt --limit 50`
   - Parse `.gitmodules` to find all submodule URLs hosted under `ArduPilot/` or `ardupilot/` on GitHub
   - For each ArduPilot-owned submodule repo, run: `gh pr list --repo <owner/repo> --label "$ARGUMENTS" --json number,title,author,url,updatedAt --limit 50`
   - Combine all results, tracking which repo each PR belongs to. Note that wiki PRs are documentation-focused (ReST under `*/source/docs/`); review for technical accuracy vs the current ArduPilot codebase, broken `:ref:` cross-references, ReST syntax, and consistency with existing wiki conventions.

2. For each PR found (in any repo):
   - **Always fetch the CURRENT diff** with `gh pr diff <number>` (add `--repo <owner/repo>` for submodule PRs). Never reuse findings carried over from a previous review of the same PR — PR state changes daily. If this PR was reviewed before (a different label, an earlier day, or any second pass), treat it as fresh: re-check every prior finding against the code that is on the branch *now*. If a prior finding has since been fixed, say so explicitly ("resolved since previous review: ...") rather than repeating it or dropping it silently.
   - Check CI status with `gh pr checks <number>` (add `--repo <owner/repo>` for submodule PRs)
   - Review the code changes for:
     - Bugs or logic errors
     - Code style issues (trailing whitespace, inconsistent indentation)
     - Missing error handling
     - Memory concerns for embedded targets
     - Alignment with ArduPilot coding patterns

3. Write the HTML report directly to `pr_reviews_$ARGUMENTS.html` in the current directory (do not read the file first - it is pure generated output). Note: the maintainer's ArduPilot setup instead writes to a fixed path per label, e.g. `/data/APM.claude/devcall_pr_reviews.html`.
   - **Put the review date prominently at the top**, as a `<p><b>Review date:</b> YYYY-MM-DD</p>` line directly under the `<h1>` — PR snapshots change daily, so a dated report can be compared against a later one.
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
     - On any PR section updated in a later pass, add a `Re-reviewed: YYYY-MM-DD` line so each review's freshness is traceable
   - Summary table at the end

4. Verdicts should be:
   - **APPROVE**: Code is correct and ready to merge
   - **COMMENT**: Minor issues that should be noted but don't block merge
   - **REQUEST CHANGES**: Bugs or significant issues that must be fixed

5. For each finding, include a direct link to the relevant line in GitHub's PR diff view using the format:
   `https://github.com/<owner>/<repo>/pull/<number>/files#diff-<filepath>R<line>`

Report the summary counts when complete: X APPROVE | Y COMMENT | Z REQUEST CHANGES
