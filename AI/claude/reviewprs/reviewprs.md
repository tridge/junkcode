# Review PRs by Label or Author

Review a set of GitHub PRs and generate an HTML report. Checks the main ArduPilot repo, the ArduPilot wiki repo, and all ArduPilot-owned submodule repos.

Run this from the root of an ArduPilot checkout (it reads `.gitmodules` in the working directory). The report is written to the repository root and works in any ArduPilot checkout, not just one.

The command is incremental: it records the head commit hash each PR was reviewed at, and on a re-run it reuses the previously-published report for any PR whose head is unchanged, only re-reviewing PRs that are new or have changed. This makes a re-run to pick up new/updated PRs very fast.

## Arguments

`$ARGUMENTS` selects which PRs to review, in one of two **modes**:

- **LABEL mode** — a GitHub label (e.g. `DevCallTopic`, `Copter`, `Plane`). Reviews every open PR
  carrying that label.
- **AUTHOR mode** — a GitHub username, optionally written `@name`. Reviews every **open** PR by that
  author that was **updated in the last 7 days**.

**Resolve the mode first and say which one you picked**, before anything else — discovery, the publish
path and the comment policy all differ:

1. A leading `@` forces AUTHOR mode; strip it and use the rest as the username.
2. Otherwise test it as a label on the main repo and look for an exact, case-insensitive match:
   `gh label list --repo ArduPilot/ardupilot --search "$ARGUMENTS" --json name --jq '.[].name'`.
   Match → LABEL mode.
3. Otherwise test it as a user: `gh api users/<arg> --jq .login`. Resolves → AUTHOR mode.
4. If neither resolves, **stop and say so**. Do not guess: a mistyped label would otherwise sweep zero
   PRs and publish a confidently empty report.

If a string is both a real label and a real username, prefer LABEL and say so, so the user can re-run
with `@name` to force the other.

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

The report file lives in the repository root, i.e. beside the checkout you ran from, whatever that is —
`$(git rev-parse --show-toplevel)/<report>`. The filename and the published location depend on the mode,
so **the two modes never overwrite each other's local file or published report**:

| | LABEL mode | AUTHOR mode |
|---|---|---|
| local file | `devcall_pr_reviews.html` | `user_pr_reviews_<user>.html` |
| published "latest" | `https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html` | `https://uav.tridgell.net/UserReviews/<USERNAME>.html` |
| dated archive | `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html` | none |
| re-run reads | the per-label latest | the per-user page |

The "latest" URL for the active mode is the one a re-run fetches to decide what to skip (step 2), and the
one step 9 publishes to. AUTHOR mode has no dated archive — the per-user page is simply kept current.

1. Find the PRs to review. **Capture each PR's current head commit** (`headRefOid`); the short hash is its first 10 characters.

   **LABEL mode** — every open PR carrying the label, across the main repo, the wiki repo and submodules
   (the per-repo commands are listed below).

   **AUTHOR mode** — every **open** PR by that author, in the same set of repos, **updated within the last
   7 days**. Use `--author` and filter on `updatedAt`:
   ```bash
   CUTOFF=$(date -u -d '7 days ago' +%Y-%m-%dT%H:%M:%SZ)
   gh pr list --repo <owner/repo> --author "<user>" --state open \
              --json number,title,author,url,updatedAt,headRefOid --limit 100 \
     | jq --arg c "$CUTOFF" '[.[] | select(.updatedAt > $c)]'
   ```
   Note the differences from LABEL mode, all of which matter:
   - `--state open` is explicit. An author sweep would otherwise pull in their merged and closed PRs,
     which is a lot of noise and nothing actionable.
   - The 7-day window is on `updatedAt`, not `createdAt` — the point is "what has this person been working
     on lately", so an old PR they pushed to yesterday belongs in, and a PR they opened last month and
     have not touched does not.
   - `--limit 100`, since a prolific author across a week can exceed the default page.
   - **Report the window explicitly** in the summary and in the report header ("open PRs updated since
     `<CUTOFF>`"), because unlike a label the set is time-dependent: the same command run tomorrow
     legitimately returns a different set, and a reader needs to know the boundary that produced it.

   **Do not use `gh api .../contents/<path>?ref=<sha>` to check a file at a PR head.** That call plus
   `--jq .content | base64 -d` fails silently — on 2026-08-04 it returned empty for an entire run, and
   the `grep -c` downstream reported `0` matches, which was read as "the symbol is gone" when it was
   still there. Use `curl -fsSL https://raw.githubusercontent.com/<owner>/<repo>/<sha>/<path>` instead,
   and **print the fetched length next to the match count** so a failed fetch cannot masquerade as a
   negative result. Applies to any "already fixed at the head" or "no longer present" claim.

   **Re-read the title (and head) immediately before writing a finding about it, and again before posting.** PR metadata drifts during a long run: on 2026-07-29 the author of `#32657` renamed it from `SIMPLIFLYH7_Board_ID_1215` to `..._1220` mid-run, so a finding written from the step-1 snapshot ("the title says 1215 but the PR adds 1220") was already wrong by the time the comment was posted, and told the author to fix something they had just fixed. Treat anything you assert *about* a PR — not just its diff — as needing a fresh read at the moment you assert it. This is the same trap as reusing a stale diff, one level up.

   **The head hash is the assertion most likely to go stale, so re-check it immediately before posting —
   not just before reviewing.** Every comment opens with "Reviewed at head `X`", which is the claim that
   scopes every finding under it. On 2026-08-18 that line was wrong on 2 of 29 PRs: `#33955` and `#34032`
   were pushed at 22:03 and the comments went out at 22:11 still quoting the step-1 hashes. Both pushes
   happened to be content-neutral (a rebase with dead-code removal, and a per-subsystem commit re-split),
   so the findings stayed valid and only the header lied — but nothing about the mechanism guarantees
   that, and a substantive push in that window means telling an author about bugs they have just fixed.
   Note the reverse case is *not* an error and should not be reported as one: on the same run `#34087` and
   `#34088` were pushed **after** the comments landed, which is ordinary PR activity.

   So immediately before posting, re-fetch `headRefOid` for every PR you are about to comment on and
   compare it with the hash the comment quotes:
   ```bash
   for n in $(cat "$SCRATCH/todo.txt"); do
     cur=$(gh pr view $n --repo <owner/repo> --json headRefOid --jq '.headRefOid[0:10]')
     want=$(grep "^$n:" "$SCRATCH/reviewed_heads.txt" | cut -d: -f2)
     [ "$cur" = "$want" ] || echo "HEAD MOVED $n: reviewed $want, now $cur"
   done
   ```
   Any line of output is a decision, not a warning. Either re-review that PR at the new head (cheap if the
   delta is small — `gh pr diff` at both heads and compare), or post with the review's own head hash and
   say plainly in the opening line that the PR has moved since, so the author knows the scope. Do not
   silently post the old hash as though it were current.
   Per-repo commands. In AUTHOR mode substitute `--author "<user>" --state open --limit 100` for
   `--label "$ARGUMENTS" --limit 50`, and apply the `updatedAt` cutoff above to each result:
   - Main repo: `gh pr list --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Wiki repo: `gh pr list --repo ArduPilot/ardupilot_wiki --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Parse `.gitmodules` to find all submodule URLs hosted under `ArduPilot/` or `ardupilot/` on GitHub
   - For each ArduPilot-owned submodule repo, run: `gh pr list --repo <owner/repo> --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Combine all results, tracking which repo each PR belongs to. Give each PR a stable **key**: the PR number for the main repo, or `<reponame>#<number>` for wiki/submodule PRs (e.g. `mavlink#360`, `wiki#7730`). Note that wiki PRs are documentation-focused (ReST under `*/source/docs/`); review for technical accuracy vs the current ArduPilot codebase, broken `:ref:` cross-references, ReST syntax, and consistency with existing wiki conventions.

2. **Fast incremental skip — decide what actually needs reviewing.** Fetch the previously-published report for this label and compare head hashes:
   - LABEL mode: `curl -fsS https://uav.tridgell.net/DevCallReviews/$ARGUMENTS/devcall_pr_reviews.html`
     AUTHOR mode: `curl -fsS https://uav.tridgell.net/UserReviews/<USERNAME>.html`
     (a 404/failure means this is the first run for this label/author — treat every PR as new and review all of them).
   - Parse its review manifest, an HTML comment of the form:
     `<!-- reviewprs-manifest v1 label="..." generated="YYYY-MM-DD" heads="<key>:<shorthash> <key>:<shorthash> ..." -->`
     giving the head each PR was last reviewed at.
   - Classify every PR found in step 1:
     - **REUSE** — key is in the manifest AND its current short head == the manifest's head. Already reviewed at this exact commit; do NOT re-review. Carry its content over verbatim in step 4.
     - **REVIEW** — key absent from the manifest (new PR) OR current head differs (changed PR). These are the only PRs that get a diff-fetch + review.
     - **DROPPED** — key in the manifest but no longer in the current labelled set (merged, closed, or label removed). Do not carry it into the open-PR sections/totals; if useful, check `gh pr view <n> --json state,mergedAt` and note it as merged/closed.
   - Report the split before proceeding, e.g. "12 labelled PRs: 2 to review (1 new, 1 changed), 10 reused, 1 dropped (merged)".
   - **Refresh CI for the REUSE set (cheap).** A reused PR's code is unchanged, but its CI result can still have changed (a flaky job re-run, or its merge-with-master base moved). For each REUSE PR run `gh pr checks <number> [--repo <owner/repo>]` (status only — do NOT re-fetch the diff or re-review). Note which reused PRs have a CI status differing from what their carried-over section currently shows.



3. **Review only the REVIEW set — and review every one of them yourself.** For each such PR (in any repo):

   **You must read every diff in the REVIEW set with your own eyes. This is not negotiable and does not
   scale down when the batch is large.** Codex is a *second* reviewer in this workflow, never a substitute
   for the first. Every PR in the REVIEW set gets both: a primary review by you (step 3) and an
   independent Codex pass (step 7). The only thing that changes this is the user explicitly asking for it
   in their message — e.g. "just do the small ones", "skip the board PRs", "codex only". Absent that
   instruction, a PR you did not read is a PR that is not ready to report on.

   **The specific failure this rule exists to prevent** (2026-08-18, `DevCallTopic`, 29 PRs): faced with a
   large batch, the run read the 14 smallest diffs and handed the other 15 to Codex as "cold reviews",
   then shipped the report with a caveat explaining that half of it was single-sourced. Two of the
   delegated PRs were 277 and 345 lines. When the delegated ones were later reviewed properly, three of
   four changed materially — one Codex REQUEST CHANGES was refuted outright (it argued against a change
   the author had made deliberately, with a comment saying why), one reported bug did not exist at all
   (measured, not argued), and on a third the most consequential defect was one *neither* reviewer had
   led with. Delegating the primary read does not merely reduce confidence; it produces wrong findings
   that would have been sent to authors. **Labelling a shortcut is not a substitute for not taking it.**

   **Parallelise your own review the same way step 7 parallelises Codex — one agent per PR.** Reading
   diffs serially in the main context is what makes a large batch feel impossible, and it is a
   self-inflicted limit: the Codex pass has always been fanned out, and there is no reason the primary
   review should not be. Launch one Claude subagent per PR in the REVIEW set, in a single message so they
   run concurrently, each doing the full step 3 job for its own PR — fetch the diff, read the whole
   comment/review thread, read surrounding source in the checkout, and verify claims rather than assert
   them. Wall-clock cost is roughly the slowest PR instead of the sum, and each agent carries only its own
   diff, so none of them is reading 40k lines of unrelated context.

   Give each agent the PR-specific angles worth chasing (a board PR wants pin/DMA/rail checks, a control
   PR wants the maths and the mode dispatch, tooling wants the failure modes of the scripts) rather than a
   generic "review this". Require every finding to be marked **VERIFIED** or **UNCONFIRMED**, and say
   explicitly that an admitted gap is worth more than a confident wrong claim, because these get sent to
   authors.

   **This does not delegate responsibility, only throughput.** Do not paste agent output into the report
   unread — that is the same failure as handing the PR to Codex, with extra steps. Re-verify every
   load-bearing finding against the source yourself before it lands: anything that drives a verdict,
   anything quantitative, and anything you would not want to defend to the author. Agents disagreeing with
   the Codex pass on the same PR is a useful signal about where to look, not something to average out.

   **When the batch is still genuinely too large to review properly, say so up front — do not silently
   ration depth.** Count the REVIEW set before starting. If even the parallel form will not cover it, tell
   the user the count and ask whether to split the run (by label subset, by repo, or oldest-first), or to
   raise the reuse window. Stopping to ask is correct; quietly downgrading half the batch to a shallower
   method and disclosing it in the report afterwards is not. The user cannot redirect a decision you made
   silently.

   - Fetch the diff with `gh pr diff <number>` (add `--repo <owner/repo>` for submodule PRs)
   - Check CI status with `gh pr checks <number>` (add `--repo <owner/repo>` for submodule PRs)
   - **Read the PR's comments and reviews before writing findings**, not just the diff:
     ```bash
     gh api --paginate repos/<owner>/<repo>/issues/<number>/comments --jq '.[] | "\(.created_at) \(.user.login): \(.body)"'
     gh api --paginate repos/<owner>/<repo>/pulls/<number>/reviews  --jq '.[] | "\(.submitted_at) \(.user.login) \(.state): \(.body)"'
     gh api --paginate repos/<owner>/<repo>/pulls/<number>/comments --jq '.[] | "\(.path):\(.line) \(.user.login): \(.body)"'
     ```
     The thread routinely holds the author's rationale for something that looks wrong in isolation, a maintainer's objection that outranks anything a review will find, hardware-test evidence that is not in the diff, and answers to questions the diff raises. Reading it does three things:
     - **Avoids repeating a point already made.** Re-raising what a maintainer said last week, or what the author already explained, wastes their time and makes the whole report look automated and unread.
     - **Avoids contradicting a decision already taken.** If a maintainer has explicitly accepted a trade-off, say so and argue against it on the merits if warranted — do not report it as a fresh defect.
     - **Catches replies aimed at a previous run's comment.** On a re-review the author may have answered the last round's findings in the thread rather than in code. Those answers must be checked against the diff like any other claim, and the finding then marked resolved, still-open, or disputed. Never silently drop a finding because the author said it was fixed, and never repeat one they have answered without engaging with the answer.
     Where a finding survives despite something in the thread, reference that explicitly ("the author notes X in the thread; that does not cover the case where…") so the author can see it was read.
   - Review the code changes for:
     - Bugs or logic errors
     - Code style issues (trailing whitespace, inconsistent indentation)
     - Missing error handling
     - Memory concerns for embedded targets
     - Alignment with ArduPilot coding patterns

4. Write the HTML report directly to the report file (`devcall_pr_reviews.html` in the repository root) — do not read the file first, it is pure generated output. Build it from the REUSE sections (copied verbatim from the fetched prior report) plus freshly-written sections for the REVIEW set:
   - As the first line inside `<body>`, emit the review manifest comment covering **every** PR in the final report (reused + reviewed): `<!-- reviewprs-manifest v1 label="$ARGUMENTS" generated="<DATE>" heads="<key>:<shorthash> ..." -->`. The next run depends on this.
   - Table of contents with author name next to each PR number and quick verdict

   - **Every table in the report must be click-to-sort.** Give each `<table>` a `sortable` class, real
     `<thead>`/`<tbody>` sections (the JS sorts `tBodies[0]`, so a bare `<tr>` of `<th>` at the top of the
     table will not work), and a one-line hint under the contents table saying headings are clickable.
     Clicking a heading sorts by that column, clicking again reverses, and the active column shows ▲/▼.
     Requirements that make it actually useful rather than decorative:
     - **Sort by meaning, not by displayed text.** Put an explicit `data-sort` attribute on any cell whose
       natural sort differs from what it shows, and have the JS prefer it over `textContent`. In practice:
       the PR column sorts on the bare number (so `#23578` does not land after `#34094` lexically), the
       verdict column on a severity rank (APPROVE=0, COMMENT=1, REQUEST CHANGES=2), and the CI column on
       the number of failing jobs, with "no CI has run" ranked −1 so unrun PRs group together. Sorting the
       verdict column alphabetically happens to give the same order today, but that is luck and breaks the
       moment a verdict is renamed — rank it explicitly.
     - **Numeric when both keys are numeric, collated otherwise**, using
       `Intl.Collator(undefined, {numeric: true})` so mixed text sorts sensibly.
     - **Stable.** Decorate rows with their original index and fall back to it when keys compare equal, so
       sorting by verdict leaves PRs in their existing order within each group.
     - **Keyboard accessible**: `tabIndex=0`, `role="button"`, Enter/Space activate, and `aria-sort` set on
       the active heading (the ▲/▼ indicator is driven off `aria-sort`, so this is not optional decoration).
     - **Self-contained**: inline `<script>`, no CDN or external library. The report is rsynced to a static
       host and read offline as often as not.

     **Write the ▲▼↕ indicators as literal characters, never as CSS `\\` escapes.** If the CSS is emitted
     from a Python string (it is, if you generate the report with a script), Python consumes `\2195` as an
     *octal* escape long before CSS sees it: `\21` becomes `chr(0x11)` and the literal text `95` is left
     behind, so every heading renders as an invisible control character followed by `95`, `B2` or `BC`.
     That shipped on 2026-08-18 and had to be spotted by eye. Either paste the glyphs in directly or
     double the backslash.

     **Verify sorting in a real browser before publishing — the comparator passing is not enough.** Run
     the sort keys through `node` first (PR numbers numeric, verdicts by severity, equal keys stable), then
     load the published page and click the headings, checking both the row order *and*
     `getComputedStyle(th, '::after').content`. The escape bug above is invisible to a comparator test and
     to structural checks of the HTML; only rendering the page catches it.
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
     - A **"Reviewed by"** line naming both passes, e.g. `Reviewed by: Claude + Codex (cross-checked)`.
       Under the step 3 rule every PR in the REVIEW set should read exactly that. If any PR would say
       anything else, **that is a bug in the run, not a caption to write** — go and do the missing pass
       before finalising. The only legitimate exception is a depth reduction the user asked for in their
       message, and then the line must say so: `Reviewed by: Codex only (at user's request)`.

   - **Completeness gate — check this before writing the report, not after.** Every PR in the REVIEW set
     must have (a) your own read of the diff and (b) a Codex result. Verify it mechanically, the same way
     the Codex logs are counted in step 7:
     ```bash
     # every REVIEW-set PR must appear in both lists
     for n in $(cat "$SCRATCH/todo.txt"); do
       [ -s "$SCRATCH/validate_$n.log" ] || echo "NO CODEX PASS: $n"
       grep -q "^$n\$" "$SCRATCH/claude_reviewed.txt" || echo "NOT READ BY CLAUDE: $n"
     done
     ```
     Append each PR number to `$SCRATCH/claude_reviewed.txt` as you finish reading its diff, so this is a
     record rather than a recollection. Any output from that loop means the run is incomplete — finish it
     or tell the user which PRs you are not covering and why, **before** publishing anything.
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

   **Codex reviews every PR in the REVIEW set, and so do you.** The two passes are additive and neither
   one substitutes for the other. Concretely, that means:
   - **Never build a Codex task for a PR you have not read yourself.** If you catch yourself writing a
     "review this diff cold and report anything wrong" task for a PR because you did not get to it, stop
     and go read the diff. That is the exact move that produced the 2026-08-18 failure described in step 3.
   - The **cold spot-check** below is for PRs you reviewed and *cleared* — its value is that the agent is
     not anchored by findings you already made. It is a check on your APPROVE verdicts, **not** a way to
     cover PRs you skipped. A cold agent pointed at an unread PR is a primary review by Codex wearing the
     spot-check's name.
   - Every PR must end the run with **both** a finding-validation (or cold spot-check) result **and** your
     own read. If any PR has only one of those when you come to write the report, the run is not finished.

   - **Run the validation as parallel per-PR agents, one Codex process per PR.** Do not send the whole batch to a single agent: it serialises what is naturally independent work, and it forces one context to hold every diff at once, which makes each individual check shallower. One agent per PR finishes in roughly the time of the slowest PR rather than the sum, and each agent carries only its own diff.

     **Use `codex exec` directly, NOT `codex-session`.** `codex-session` stores a resumable session id keyed by the working directory (`$CODEX_HOME/.claude-threads/<hash>.sid`); N processes launched from the same directory race on that file and can resume each other's threads. These validations are one-shot and need no continuity, so invoke the raw form, which writes no session state:
     ```bash
     codex exec --skip-git-repo-check "$TASK" </dev/null
     ```

     Fan them out with a bounded worker pool (6 is a reasonable cap — beyond that you are mostly competing for API rate limit), each writing to its own log.

     **The pool MUST be detached with `setsid nohup` and MUST signal completion with a sentinel
     file.** Do not launch it with the Bash tool's `run_in_background`, and do not write the worker
     as an exported shell function. Both failure modes were hit on 2026-08-04 and each silently
     produced a partial run — see the note below. Put the worker in its own script file:

     ```bash
     # $SCRATCH/worker.sh — a real file, not an exported function
     cat > "$SCRATCH/worker.sh" <<'EOS'
     #!/bin/bash
     n=$1; safe=${n//[^0-9A-Za-z]/_}
     codex exec --skip-git-repo-check "$(build_task "$n")" </dev/null \
         > "$SCRATCH_DIR/validate_$safe.log" 2>&1
     EOS

     rm -f "$SCRATCH/POOL_DONE"
     setsid nohup bash -c "
        export SCRATCH_DIR=$SCRATCH
        cat $SCRATCH/todo.txt | xargs -P 6 -I{} bash $SCRATCH/worker.sh {}
        touch $SCRATCH/POOL_DONE
     " </dev/null > "$SCRATCH/pool.log" 2>&1 &
     disown 2>/dev/null

     # wait for it — poll the SENTINEL, never the process table
     until [ -f "$SCRATCH/POOL_DONE" ]; do sleep 30; done
     ```

     Two traps, both of which cause a partial run that looks like a complete one:

     - **A backgrounded `xargs -P` pool dies with its launcher.** Started via `run_in_background`,
       the tool reported "completed" as soon as the wrapper returned, having launched only the first
       6 of 21 agents; the remaining 15 never ran. The logs that *did* exist looked fine, so the
       shortfall is invisible unless you count them. `setsid nohup` + `disown` survives this.
     - **`pgrep -x codex` is NOT a completion signal.** It matches every Codex process on the
       machine, including other Claude sessions', so it reports "still running" long after your own
       pool has died and "finished" is never reliable either. Poll the sentinel file instead.

     Then **count the logs against the input set before using any of them**, and re-run the
     stragglers — a missing agent is a PR that received no validation at all:
     ```bash
     for n in $(cat "$SCRATCH/todo.txt"); do [ -s "$SCRATCH/validate_$n.log" ] || echo "MISSING $n"; done
     ```

     Note `codex exec` logs interleave the agent's tool transcript with its prose, and the final
     answer is **not** reliably the tail of the file. Extract findings by grepping for the
     `BUG`/`ISSUE`/`NOTE` markers you asked for rather than by slicing the end of the log.

     Measured on 2026-07-29: three agents launched this way completed in **47 s** wall-clock, against roughly 3.5 min per PR when the same work was done serially in one agent. Verified that concurrent `codex exec` runs leave `~/.codex/.claude-threads/` byte-identical — no session state is written, so there is nothing to race on.

     Each **finding-validation** agent is told to handle exactly one PR, and is given only that PR's findings — not the whole report. Its task should instruct it to:
     - Fetch that PR's diff itself (`gh pr diff <number> [--repo <owner/repo>]`, plus `gh pr checks` / `gh pr view` as needed). Give it the PR number and the findings inline; do **not** point it at the HTML report, so it cannot be primed by the other PRs' conclusions.
     - Verify for each finding: (a) the issue is real, (b) the `file:line` reference is correct and still present at the PR's current head, (c) the severity and the PR's overall verdict are appropriate.
     - Call out **false positives**, **mislocated** `file:line` references, **wrong verdicts**, and any **significant issues the review missed**.
     - Output a per-finding assessment using **CONFIRM / REFUTE / ADJUST** (corrected detail for ADJUST), plus any **NEW** findings.

     Keeping the agents single-PR also keeps the failure modes independent: one agent timing out or going off the rails costs you that PR's validation, not the whole pass. Check every log came back non-empty and re-run any that didn't, rather than silently reporting fewer validations than PRs.

   - **APPROVE spot-check — validate the PRs you cleared, not just the findings you made.** Everything above checks claims you *made*, so a PR you wrongly waved through is invisible to validation by construction: no finding, nothing to cross-check. That is the workflow's blind spot, and it is where a missed bug is most likely to survive to merge. So in addition to the finding-level pass, pick from the reviewed set the **APPROVE PRs that carry real risk** — non-trivial logic changes, concurrency or state machines, anything touching caching/lifetime/ownership, new board/hwdef definitions (pin labels, GPIO numbering and power-rail defaults are silently wrong in ways that still compile), and anything whose diff newly depends on existing shared state — and have Codex review those diffs **cold**, as a fresh reviewer with no knowledge of your findings. Do *not* show it your review of those PRs; ask only "review this diff and report anything that looks wrong". A clean APPROVE with three or four NOTEs and a well-written explanatory comment is not evidence of correctness — a good comment explaining one limitation is a known trap that stops the reader hunting for an unexplained one. Prefer depth over breadth: two or three risky APPROVEs traced end-to-end beats a shallow pass over all of them. If a spot-check turns up a real problem, move that PR out of APPROVE and treat it like any other finding (report section, verdict, tables, totals, and a posted comment per step 8).

     **Run these as their own parallel agents, launched alongside the finding-validation ones**, using the same bounded pool and the same raw `codex exec` invocation. Keep the two kinds strictly separate — a cold agent must never receive the report, the findings, or the verdict for its PR, because the whole value of the check is that it has not been anchored by them. In practice that means: build the cold task from the PR number alone, and never let a PR's finding-validation task and its cold task share a process. It is fine for the same PR to have both (a cold agent and a finding-validation agent) running concurrently — they are independent.

     This blind spot is not hypothetical. On 2026-07-29 the cold check caught four real defects in a new-board hwdef (`#33423`) that the primary review had cleared as APPROVE with no findings at all — an invalid `HAL_HEATER_GPIO_PIN`, a safety-switch pin label that never generates the define the code tests for, power rails initialised off, and a `BATT_MONITOR` line the hwdef parser does not recognise. Nothing else in this workflow would have surfaced any of them.

   - **Collect and aggregate** once the pool drains: read every `validate_*.log` and `cold_*.log`, and tally CONFIRM / ADJUST / REFUTE / NEW across all of them so the counts in the report's validation line are real rather than estimated. Note that with parallel agents no single log contains the whole picture, so the aggregation step is not optional — do not summarise from whichever log you happened to read last.

   - Then **evaluate Codex's assessment with your own judgement — do not blindly accept it.** Codex is a second opinion, not an authority: re-check anything it disputes against the actual diff before acting. Treat agreement between you and Codex as higher-confidence, and treat any contradiction with claims made earlier in the session as something to flag, not silently resolve. **This applies just as much when it refutes you** — verify the refutation against the source before withdrawing a finding, and equally verify a cold agent's new bugs before reclassifying a PR on them. Both directions were exercised on 2026-07-29 and both held up, but the point is that they were checked rather than assumed.

     Three failure modes seen repeatedly in Codex output, all of which survive into the report unless you
     check (all three hit on 2026-08-18):
     - **It skews heavily to REQUEST CHANGES.** In that run 13 of 17 cold agents returned REQUEST CHANGES.
       The verdict label carries much less information than the evidence under it; re-derive the verdict
       from the findings that survive your own check, never copy it across.
     - **It argues against deliberate, commented changes.** On `#33977` it reported a dropped-entry bug in
       code the author had just changed on purpose, with a comment in the diff explaining the reason.
       **Before accepting a finding, check whether the diff's own comments already answer it** — if the
       author addressed the point, the finding must engage with their reasoning or be dropped. Applying
       that one as prescribed would have reintroduced the bug being fixed.
     - **Quantitative claims are often wrong in detail even when the concern is real.** On `#33979` a
       claimed 101° phase error from float32 time measured out at ≤0.06°. **Any numeric claim — a
       magnitude, a timing, a size, a count — gets reproduced independently before it goes in the report.**
       A ten-line script settles it; on 2026-08-18 that method confirmed the `#23578` 57.3× timeout error
       and the `#34030` version mismatch, and refuted the `#33979` one.

     **Know the project's normal practice before calling something a process violation.** A PR whose
     submodule pointer moves to an unmerged commit is **normal and expected** in ArduPilot, not a defect:
     submodule changes land in their own repo's PR, and the parent PR legitimately points at it while both
     are in review. The only expectation is that the submodule PR is **linked in the description**. On
     2026-08-18 this was reported as a blocking merge gate on `#34087` and written up as a BUG driving
     REQUEST CHANGES — the description already said "Depends on ArduPilot/ChibiOS#110 and
     ArduPilot/mavlink#517", so the requirement was met and there was nothing to fix. If the links are
     genuinely absent, ask for them in a NOTE; never treat submodule dependency itself as a bug.
     The general lesson: before escalating anything to BUG or REQUEST CHANGES on *process* grounds rather
     than code grounds, check whether it is simply how the project works.

   - **Modify the report based on that evaluation:** remove or correct findings Codex refuted (and you agree are wrong), fix mislocated `file:line` references, adjust severities and any affected PR verdicts, and add any validated NEW findings. Where you disagree with Codex after re-checking, keep your finding but add a one-line note of the disagreement and why.

   - Record the validation in the report itself: add a short **"Codex validation"** line near the top (date + one-line outcome, e.g. "Codex cross-checked N findings: X confirmed, Y adjusted, Z refuted, W added; spot-checked K APPROVE PRs cold, J moved out of APPROVE"). Name which APPROVE PRs were spot-checked, so a reader can see which clean verdicts were independently tested and which were taken on one reviewer's word. If any verdicts changed, update the per-PR sections, the contents/quick-verdict table, the summary table, and the final totals so the whole report stays consistent.

8. **Post review comments to the PRs — LABEL mode, DevCallEU and DevCallTopic only.** This step runs automatically when the mode is LABEL and the label is exactly **`DevCallEU`** or **`DevCallTopic`**. For every other label, do **not** post any comments unless the user explicitly asks you to in their message. (When they do ask for another label, follow the same mechanics below.)

   **AUTHOR mode does not post, ever, unless explicitly asked in the user's message.** The default is a
   report only. The dev-call labels are a standing, publicly-understood process — an author sweep is not:
   it is a lens someone chose to point at a particular person, the PRs in it have not been put forward for
   review by anyone, and several may be drafts or work the author has not asked anybody to look at.
   Auto-commenting on every recently-touched PR by one person would be both surprising and, aimed at
   someone other than the person running the command, pointed. If the user does ask, follow the same
   mechanics below unchanged.

   Posting happens **after** the report is finalised, i.e. only once a PR has been reviewed by **both you and Codex** (steps 3 and 7). Therefore comments are posted for the **REVIEW set** (the PRs reviewed this run) — reused PRs were already commented in the prior run at the same head, so skip them.

   - **Scope — comment on every reviewed PR, including clean APPROVEs.** Post a comment for each PR in the REVIEW set. Do not stay silent on a PR just because it came out clean: an author whose PR was reviewed and cleared should be told so, otherwise silence is ambiguous between "reviewed, fine" and "nobody looked". This applies to a clean APPROVE with no actionable findings as much as to a PR with a list of bugs.
     Note the asymmetry this addresses — APPROVE is both the least-validated verdict (no findings for Codex to cross-check, hence the cold spot-check in step 7) and the one that historically produced no author-facing output, so a wrongly-cleared PR was invisible from both directions. Commenting on it puts the clearance on the record where the author can push back on it.
   - **Content:** mirror that PR's findings from the finalised report — verdict, then findings grouped by severity, each with its `file:line` reference and a one-line description, plus suggested fixes where useful. Use the post-Codex findings (refuted ones removed, line refs corrected). If you dropped a finding as a false positive during validation, note that briefly so the author isn't left chasing it. Where there *are* actionable findings, drop purely confirmatory notes from the comment body — they belong in the report as evidence of what was checked, but in a comment they bury the actionable items.
     On an **APPROVE**, open with that plainly ("no blockers") so the author is not left guessing whether the comment is a merge objection.
     For a **clean APPROVE with nothing actionable**, the comment is short and its job is to say what was actually checked, not to pad. Name the specific things verified — the paths traced, the callers audited, whether a cold review was run and what it looked for — so the author can judge how much the clearance is worth and challenge it if a risk was missed. A bare "looks good to me" is worse than nothing, because it claims review effort without evidencing any. Keep it to a few lines.
   - **Mark every comment as AI-generated.** Begin the body with a marker line, e.g.: `**Automated review note — AI-generated (Claude), validated against the live diff.** Please sanity-check before acting.`
   - **Update, don't duplicate — unless the update would be buried.** If you have never commented on that PR, post a new comment. If you have already posted an AI-generated comment, the choice between editing it and posting a second one is decided entirely by the test below: edit in place with `gh pr comment <number> [--repo <owner/repo>] --edit-last --body-file <file>`, or PATCH it by id if `--edit-last` is not viable.

     **The exception: deprecate-and-repost.** Editing in place has a failure mode — the edited comment stays at its original position in the thread. If the discussion has moved on since, the updated review sits far up the page where nobody sees it, and the author has no notification that it changed.

     **The test is simply whether your existing comment is still the last thing on the PR:**

     - **Your AI comment is the most recent comment** ⇒ **edit it in place**, replacing the previous body. It is still at the bottom of the thread, so an edit is seen and a second comment would just be noise.
     - **Anything has been posted since it** — an issue comment, a review, or a review comment, by anyone other than you ⇒ **deprecate-and-repost** as below, so the new review lands at the bottom and generates a notification.

     Timing does not enter into it. A same-day re-run where somebody has commented in between still gets a new comment, and a week-old comment that is still the last thing on the PR still gets edited in place. What matters is only whether an in-place edit would be buried.

     When the test says repost, instead of a silent in-place edit:
     - **PATCH the old comment** to mark it deprecated. Prepend a marker line and collapse the original body so the record survives without adding noise:
       ```markdown
       > **Deprecated — see below for the updated review.**

       <details><summary>Previous review (2026-08-05)</summary>

       ...original body...

       </details>
       ```
     - **Then post the new review as a fresh comment**, so it lands at the bottom of the thread and generates a notification.

     Determining it — count anything by anyone else newer than your comment, across all three sources, with `--paginate` on each (a review comment or a review counts just as much as an issue comment):
     ```bash
     ME=$(gh api user --jq .login)
     MINE=$(gh api --paginate repos/<owner>/<repo>/issues/<n>/comments \
       --jq "[.[] | select(.user.login==\"$ME\") | select(.body|test(\"AI-generated\"))] | last")
     MY_ID=$(jq -r .id <<<"$MINE"); MY_AT=$(jq -r .created_at <<<"$MINE")
     NEWER=$( { gh api --paginate repos/<owner>/<repo>/issues/<n>/comments \
                   --jq ".[] | select(.user.login!=\"$ME\") | .created_at"
                 gh api --paginate repos/<owner>/<repo>/pulls/<n>/comments \
                   --jq ".[] | select(.user.login!=\"$ME\") | .created_at"
                 gh api --paginate repos/<owner>/<repo>/pulls/<n>/reviews \
                   --jq ".[] | select(.user.login!=\"$ME\") | .submitted_at"; } \
               | while read -r d; do [ "$d" \> "$MY_AT" ] && echo x; done | wc -l)
     ```
     (That loop deliberately avoids awk's whole-record variable — dollar-zero. When this file is invoked
     as a slash command the runner substitutes positional parameters, so a literal dollar-zero written in
     a snippet is rewritten to the command's argument: `awk 'DevCallTopic > t'`, which is still valid awk,
     always false, and fails silently. Only dollar-zero is affected — the dollar-one in the `worker.sh`
     heredoc above survives substitution and is needed there, so leave it alone.)
     `NEWER == 0` ⇒ your comment is still last ⇒ **edit in place**.
     `NEWER > 0` ⇒ something came after it ⇒ **deprecate-and-repost**.

     Note this is a per-PR decision made at posting time, so compute it per PR rather than picking one
     mode for the whole run — in a typical batch some PRs will be quiet and get edits while others have
     moved on and get fresh comments.

     When you do repost, say so in the new comment's opening line — e.g. "Re-reviewed at head `<sha>`; my earlier comment above is superseded" — and if a finding from the previous round is still unaddressed, mark it as re-raised rather than presenting it as new. An author who has already read that point once deserves to know you know that.

     **Always pass `--paginate` when listing comments.** `gh api repos/<owner>/<repo>/issues/<number>/comments` returns only the first 30, and an active PR easily exceeds that — your own comment is the *newest*, so it is exactly the one that falls off page 1. Without it the lookup silently reports "no prior comment" on the busiest PRs and you post a duplicate instead of editing:
     ```bash
     gh api --paginate repos/<owner>/<repo>/issues/<number>/comments \
       --jq '.[] | select(.user.login=="<you>") | select(.body|test("AI-generated \\(Claude\\)")) | .id'
     ```
     The same applies to any post-run verification count — note that with `--paginate` a `--jq` expression returning a per-page aggregate (e.g. `| length`) emits **one result per page**, so sum them (`| paste -sd+ | bc`) rather than reading the first number.
   - Add `--repo <owner/repo>` for wiki/submodule PRs.
   - Record in the report (near the top, alongside the Codex line) which PRs got a comment posted/updated, with links to the comments. Distinguish the three cases — **posted** (first comment), **edited in place**, and **deprecated + reposted** — so a reader can tell which PRs now carry two AI comments and why.

9. **Publish the report to the web.** Once the report is finalised (including the Codex revisions and any comment posting), publish it to BOTH the per-label "latest" directory (which the next re-run reads) and a dated archive directory. This is automatic — always do it at the end of the run.

   ```bash
   # LABEL mode
   REPORT="$(git rev-parse --show-toplevel)/devcall_pr_reviews.html"
   LABEL="$ARGUMENTS"
   DATE=$(date +%Y_%m_%d)   # e.g. 2026_06_09 — must match the report's review date
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$LABEL/   # per-label latest
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$DATE/    # dated archive

   # AUTHOR mode — a single page per user, kept current; no dated archive.
   # Note the destination is a FILENAME, not a directory: the trailing path component
   # is <USERNAME>.html, so rsync must be given that exact target path.
   USER_NAME=<username>
   REPORT="$(git rev-parse --show-toplevel)/user_pr_reviews_${USER_NAME}.html"
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/UserReviews/${USER_NAME}.html
   ```

   - LABEL mode serves at `https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html`, with the dated snapshot at `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html`.
   - AUTHOR mode serves at `https://uav.tridgell.net/UserReviews/<USERNAME>.html`. Because it is one page per user rather than a directory, publishing **replaces** the previous run's page — that is intended, since the 7-day window means an archive of author sweeps would mostly be duplicates. Print the URL when done, as for LABEL mode.
   - `--mkpath` creates the destination directory if it doesn't exist (the `UAV-web/DevCallReviews/` parent already exists). If a host's rsync predates 3.2.3 and lacks `--mkpath`, first `ssh tridgell.net mkdir -p UAV-web/DevCallReviews/<dir>`, then rsync without `--mkpath`.
   - Use the SAME `<DATE>` as the report's review date so the directory date and the in-report date agree.
   - Print both public URLs when done.

Report the summary counts when complete. Say which **mode** was used and what selected the set — the
label, or the username plus the `updatedAt` cutoff that produced it. Note the skip split and validation
outcome (including which APPROVE PRs were spot-checked cold), any PR comments posted/updated, and give
the published URL:

X APPROVE | Y COMMENT | Z REQUEST CHANGES (reused N, reviewed M; Codex: X confirmed / Y adjusted /
Z refuted / W added, K APPROVE spot-checked / J reclassified; comments posted on P PRs) — published at
`https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html` (LABEL mode) or
`https://uav.tridgell.net/UserReviews/<USERNAME>.html` (AUTHOR mode).

In AUTHOR mode, if the sweep returns **no** PRs, say that plainly — "no open PRs by `<user>` updated
since `<cutoff>`" — and do not publish an empty page over a previously useful one.
