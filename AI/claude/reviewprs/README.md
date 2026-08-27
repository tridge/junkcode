# /reviewprs

A Claude Code slash command that reviews a set of open GitHub PRs — selected by
**label**, by **author**, as a **follow-up** to earlier reviews, or against a separate
project (the **rsync** target) — and writes a single self-contained **HTML report** with
per-PR findings and an APPROVE / COMMENT / REQUEST CHANGES verdict. Built for the
ArduPilot dev call workflow: it sweeps the **main repo, the wiki repo, every
ArduPilot-owned submodule** (parsed from `.gitmodules`), and the standalone ArduPilot
repos (`SupportProxy`, `pymavlink`, `useralerts`, `MissionPlanner`, `CustomBuild`,
`MethodicConfigurator`, `ArduRemoteID`), so one command covers the whole tree.

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
/reviewprs                  # no argument: the triple-run — DevCallTopic, then DevCallEU, then followup
/reviewprs DevCallTopic     # label mode: every open PR with that label
/reviewprs Copter
/reviewprs @tridge          # author mode: that author's open PRs, updated in the last 7 days
/reviewprs peterbarker
/reviewprs followup         # follow-up mode: re-review PRs already commented on whose code has moved
/reviewprs rsync            # rsync mode: open AIReview PRs on RsyncProject/rsync (a separate target)
```

`$ARGUMENTS` picks one of four modes, resolved automatically:

- **no argument** → the **triple-run**: `DevCallTopic`, then `DevCallEU`, then `followup`, back to back
  (three sequential label/label/follow-up sweeps), producing up to three sets of pages plus comments;
- **`followup`** and **`rsync`** are reserved words checked first (so they never mis-resolve to a label or
  a stranger's username — `FollowUp` and `rsync` are real GitHub logins);
- a leading **`@`** forces author mode; otherwise the argument is tried as a **label** first and then as a
  **user**, and the command stops rather than guessing if it is neither (a mistyped label would otherwise
  publish a confidently empty report).

Author mode selects **open** PRs only, filtered on `updatedAt` within 7 days ("what has this person been
working on lately"). **Follow-up mode** re-reviews only PRs already reviewed-and-commented whose head has
moved since that comment — designed to be run often and do nothing when nothing has changed. **rsync mode**
is a wholly separate target (repo `RsyncProject/rsync`, label `AIReview`, its own single-page report) with
ArduPilot house rules turned off.

Run it from the root of an ArduPilot checkout. Output goes to the **repository root**
(`git rev-parse --show-toplevel`) — `devcall_pr_reviews.html` in label/follow-up mode,
`user_pr_reviews_<user>.html` in author mode, `rsync_pr_reviews.html` in rsync mode. The report has a click-to-sort table of contents
with author, verdict, reviewers and CI status; per-PR changed-file links; file:line
findings that deep-link into GitHub's diff view; and a summary table. Submodule and
wiki PRs are prefixed with their repo (e.g. `[ChibiOS] #123`, `[wiki] #7730`).

## Pipeline

1. **Find** the PRs — every open PR with the label, every open PR by the author
   updated in the last 7 days, the follow-up set, or the rsync target's `AIReview` PRs —
   across main + wiki + ArduPilot submodules + `SupportProxy` (or the single rsync repo),
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
<!-- reviewprs-manifest v1 author="tridge"      generated="2026-06-09" heads="34088:de0f387af8 ..." -->
```

On a re-run each PR still in the selected set is **reused** if its head is unchanged
(the section was already reviewed and validated at that exact commit), **reviewed** if
it is new or its head moved, or **dropped** if it left the set — merged, closed, lost
the label, or in author mode simply fell outside the 7-day window. So a re-run pays
only for what actually changed. Note that "dropped" is not a synonym for "merged":
check and report which, since a PR that merely lost its label is still open and its
findings still stand.

## Posting comments back to the PRs

Runs automatically for the `DevCallEU`, `DevCallTopic`, and `AIReview` labels, in **follow-up mode** (posting
*is* its purpose — always a fresh notifying comment), and in **rsync mode** (`RsyncProject/rsync`, an opted-in
project); any other label posts nothing unless you ask. The `AIReview` label on an ArduPilot PR is the same
opted-in signal ArduPilot shares with the rsync repo — someone applied it to ask for an auto-posted review. **Author mode never posts unless explicitly
asked** — the dev-call labels are a standing, publicly-understood process, whereas an author sweep is a
lens someone chose to point at a particular person, over PRs nobody put forward for review. Every reviewed
PR gets a comment, **including clean
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

The finished report is rsynced to a web host, to a path that depends on the mode:

- label mode, **latest** — `.../DevCallReviews/<LABEL>/devcall_pr_reviews.html`
  (canonical current report; the one re-runs read for the skip check);
- label mode, dated **archive** — `.../DevCallReviews/<DATE>/devcall_pr_reviews.html`, where `<DATE>` for
  the two dev-call labels is the **upcoming call date** in Canberra time (Tuesday for `DevCallTopic`,
  Wednesday for `DevCallEU`), so a review published early still lands where people look on the day;
- author mode — `.../UserReviews/<USERNAME>.html`, a single page per user kept current, with no dated
  archive (with a 7-day window an archive would be mostly duplicates);
- follow-up mode — its own run report under `.../DevCallReviews/followups/<DATE_TIME>/`, **plus** an
  in-place refresh of every per-label report that contained a re-reviewed PR (so their manifests stay
  truthful and a later label run doesn't redo the work);
- rsync mode — `.../RsyncReviews/index.html`, one living page listing every currently-open `AIReview` PR,
  replaced each run (closed / merged / de-labelled PRs simply drop off), no dated archive.

The modes use different local filenames and different published paths, so they never overwrite each
other.

The reports are public and **auto-generated — they may contain errors**; always
confirm against the actual PR before acting.

## Adapting to a non-ArduPilot repo

The structure is generic. The ArduPilot specifics are: the wiki repo
(`ArduPilot/ardupilot_wiki`), the submodule sweep (repos owned by `ArduPilot/`), the
embedded-target review lens (flash/RAM, 16-character parameter names, per-subsystem
commits), the publish host, and the labels that auto-post. Retarget those and the
rest — label filter, head-hash skip, parallel dual review, verdict report, comment
posting — works for any GitHub project. **rsync mode is exactly such a retarget, built
in:** a single non-ArduPilot repo (`RsyncProject/rsync`), its own label (`AIReview`), its
own single-page report, and a C / wire-protocol review lens instead of the embedded one —
a worked example of pointing the same machinery at an unrelated project.

One portability note if you edit the command file: the slash-command runner
substitutes positional parameters into the prompt, so a literal dollar-zero written
inside a shell snippet is rewritten to the command's argument. That silently turned
an `awk` record test into a always-false comparison. Avoid it in examples.
