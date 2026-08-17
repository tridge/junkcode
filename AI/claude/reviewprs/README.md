# /reviewprs

A Claude Code slash command that reviews every open GitHub PR carrying a given
**label** and writes a single self-contained **HTML report** with per-PR findings
and an APPROVE / COMMENT / REQUEST CHANGES verdict. Built for the ArduPilot dev
call workflow: it sweeps the **main repo, the wiki repo, and every ArduPilot-owned
submodule** (parsed from `.gitmodules`) for that label, so one command covers the
whole tree.

Every PR gets **two independent reviews** — Claude's own, and a cross-check by a
second AI reviewer (OpenAI Codex) — and the report is **published to the web** and
**posted back to the PRs** as clearly-marked comments.

## Install

```sh
cp reviewprs.md ~/.claude/commands/reviewprs.md         # user-global, or
cp reviewprs.md <project>/.claude/commands/reviewprs.md # per-project
```

Requires the `gh` CLI authenticated (`gh auth status`), the `codex` CLI on `$PATH`,
and `rsync`/ssh access to a web host. The command allows read-only `gh`/`git`
subcommands plus `codex`/`curl`/`rsync` and `gh pr comment` — it never pushes code.

## Usage

```
/reviewprs DevCallTopic
/reviewprs Copter
```

`$ARGUMENTS` is the label. Run it from the root of an ArduPilot checkout. Output is
written to `devcall_pr_reviews.html` in the **repository root**
(`git rev-parse --show-toplevel`). The report has a click-to-sort table of contents
with author, verdict, reviewers and CI status; per-PR changed-file links; file:line
findings that deep-link into GitHub's diff view; and a summary table. Submodule and
wiki PRs are prefixed with their repo (e.g. `[ChibiOS] #123`, `[wiki] #7730`).

## Pipeline

1. **Find** every PR with the label across main + wiki + ArduPilot submodules,
   capturing each PR's current head commit.
2. **Fast incremental skip** — fetch the previously-published report, parse its
   manifest and compare head hashes. Only new/changed PRs are reviewed; CI status is
   refreshed for the reused ones (cheap, and it does change).
3. **Review, one agent per PR, in parallel.** Each agent fetches its own diff, reads
   the whole comment/review thread, reads surrounding source, and marks each finding
   VERIFIED or UNCONFIRMED.
4. **Codex validation, also one agent per PR, in parallel** — CONFIRM / REFUTE /
   ADJUST / NEW against the findings, plus cold spot-checks of risky APPROVE verdicts.
5. **Reconcile** — every verdict-driving or quantitative finding is re-verified
   against the source before it lands in the report.
6. **Write** the HTML report (reused sections carried over verbatim + fresh ones).
7. **Post PR comments** and **publish** the report (see below).

## The rules that matter

These are the ones that were learned by getting them wrong. They are worth keeping
if you adapt this.

- **Both reviewers, every PR.** Codex is a *second* reviewer, never a substitute for
  the first. A "cold" Codex pass pointed at a PR nobody read is a primary review
  wearing a spot-check's name. There is a mechanical completeness gate before the
  report is written: any PR missing either pass fails the run.
- **Parallelise your own review too.** Reading diffs serially in one context is what
  makes a large batch feel impossible and tempts you into rationing depth. Fan out.
  If even the parallel form won't cover the batch, *say so up front* and ask how to
  split it — quietly downgrading half the batch and disclosing it afterwards denies
  the user the chance to redirect.
- **Delegation moves throughput, not responsibility.** Agent output is not pasted in
  unread; load-bearing findings get re-verified against the source.
- **Reproduce every numeric claim.** A magnitude, timing, size or count goes in the
  report only after being independently reproduced. In one run this confirmed a 57.3x
  timeout error and a release-version mismatch, and refuted a claimed 101 degree
  phase error that measured 0.06.
- **Check the diff's own comments before accepting a finding.** A second opinion will
  happily report a "bug" in code the author changed deliberately, with a comment in
  the diff saying why. Acting on one such report would have reintroduced the bug it
  was meant to fix.
- **Re-read the title and head immediately before posting**, not just before
  reviewing. Both drift during a long run, and the head hash is the claim that scopes
  every finding under it.

## Incremental re-runs (head-hash skip)

Every PR section records the head commit short hash it was reviewed at, and the
report embeds a machine-readable manifest near the top:

```
<!-- reviewprs-manifest v1 label="DevCallTopic" generated="2026-06-09" heads="33372:69bff866c7 ..." -->
```

On a re-run each currently-labelled PR is **reused** if its head is unchanged (the
section was already reviewed and validated at that exact commit), **reviewed** if it
is new or its head moved, or **dropped** if it merged, closed or lost the label. So a
re-run pays only for what actually changed.

## Posting comments back to the PRs

Runs automatically for the `DevCallEU` and `DevCallTopic` labels; any other label
posts nothing unless you ask. Every reviewed PR gets a comment, **including clean
APPROVEs** — silence is ambiguous between "reviewed, fine" and "nobody looked", and
an APPROVE is the verdict least likely to have been checked, so it says specifically
what was verified. All comments are marked AI-generated.

Whether an update edits or reposts is decided per PR, at posting time, by one test:

- **your previous comment is still the newest thing on the PR** → edit it in place;
- **anyone has posted since** → mark the old one deprecated (collapsed, so the record
  survives) and post a fresh comment, so it lands at the bottom and notifies.

Timing plays no part — a same-day re-run with intervening discussion still reposts,
and a week-old comment that is still last still gets edited. What matters is only
whether an in-place edit would be buried.

## Publishing

The finished report is rsynced to a web host in two locations:

- per-label **latest** — `.../DevCallReviews/<LABEL>/devcall_pr_reviews.html`
  (canonical current report; the one re-runs read for the skip check);
- dated **archive** — `.../DevCallReviews/<DATE>/devcall_pr_reviews.html`.

The reports are public and **auto-generated — they may contain errors**; always
confirm against the actual PR before acting.

## Adapting to a non-ArduPilot repo

The structure is generic. The ArduPilot specifics are: the wiki repo
(`ArduPilot/ardupilot_wiki`), the submodule sweep (repos owned by `ArduPilot/`), the
embedded-target review lens (flash/RAM, 16-character parameter names, per-subsystem
commits), the publish host, and the two labels that auto-post. Retarget those and the
rest — label filter, head-hash skip, parallel dual review, verdict report, comment
posting — works for any GitHub project.

One portability note if you edit the command file: the slash-command runner
substitutes positional parameters into the prompt, so a literal dollar-zero written
inside a shell snippet is rewritten to the command's argument. That silently turned
an `awk` record test into a always-false comparison. Avoid it in examples.
