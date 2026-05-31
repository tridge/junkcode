# /reviewprs

A Claude Code slash command that reviews every open GitHub PR carrying a given
**label** and writes a single self-contained **HTML report** with per-PR findings
and an APPROVE / COMMENT / REQUEST CHANGES verdict. Built for the ArduPilot dev
call workflow: it sweeps the **main repo, the wiki repo, and every ArduPilot-owned
submodule** (parsed from `.gitmodules`) for that label, so one command covers the
whole tree.

## Install

```sh
cp reviewprs.md ~/.claude/commands/reviewprs.md         # user-global, or
cp reviewprs.md <project>/.claude/commands/reviewprs.md # per-project
```

Requires the `gh` CLI authenticated (`gh auth status`) with access to the repos.
The command only allows read-only `gh`/`git` subcommands (list/diff/checks/view/
api, log/diff/show/config) — it inspects PRs, it does not post or push anything.

## Usage

```
/reviewprs DevCallTopic
/reviewprs Copter
```

`$ARGUMENTS` is the label. Output is written to `pr_reviews_<label>.html` in the
current directory (the maintainer's ArduPilot setup hard-codes a fixed path per
label instead, e.g. `/data/APM.claude/devcall_pr_reviews.html`). The report has a
table of contents with author + quick verdict, per-PR CI status, changed-file
links, file:line findings that deep-link into GitHub's diff view, and a summary
table; submodule/wiki PRs are prefixed with their repo (e.g. `[ChibiOS] #123`,
`[wiki] #7730`). It prints `X APPROVE | Y COMMENT | Z REQUEST CHANGES` when done.

## Two behaviours worth knowing (learned the hard way)

- **Always re-fetches the current diff.** On a re-review (same PR under a different
  label or on a later day) it re-checks every prior finding against what's on the
  branch *now*, and calls out anything already fixed as "resolved since previous
  review" rather than repeating stale issues. A stale review that implies fixed
  problems still exist is worse than no review.
- **Stamps the review date** prominently under the report title, and adds a
  `Re-reviewed: YYYY-MM-DD` line on any PR section updated in a later pass — PR
  state changes daily, so reports need to be dateable and comparable.

## Adapting to a non-ArduPilot repo

The structure is generic; the ArduPilot specifics are: the wiki repo
(`ArduPilot/ardupilot_wiki`), the submodule sweep (owned by `ArduPilot/`), and the
embedded-target review lens (flash/RAM concerns, coding patterns). Drop or retarget
those and the rest (label filter → per-PR diff/CI review → HTML verdict report)
works for any GitHub project.
