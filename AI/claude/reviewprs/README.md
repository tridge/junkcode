# /reviewprs

A Claude Code slash command that reviews every open GitHub PR carrying a given
**label** and writes a single self-contained **HTML report** with per-PR findings
and an APPROVE / COMMENT / REQUEST CHANGES verdict. Built for the ArduPilot dev
call workflow: it sweeps the **main repo, the wiki repo, and every ArduPilot-owned
submodule** (parsed from `.gitmodules`) for that label, so one command covers the
whole tree. It then has the findings **cross-checked by a second AI reviewer
(OpenAI Codex)** and **publishes the report to the web**.

## Install

```sh
cp reviewprs.md ~/.claude/commands/reviewprs.md         # user-global, or
cp reviewprs.md <project>/.claude/commands/reviewprs.md # per-project
```

Requires the `gh` CLI authenticated (`gh auth status`), the `codex` CLI (via the
`codex-session` wrapper, see the `codex/` command), and `rsync`/ssh access to the
web host. The command only allows read-only `gh`/`git` subcommands plus
`codex`/`curl`/`rsync` and `gh pr comment` — it never pushes to GitHub, and only
posts PR comments for the `DevCallEU` label (see below).

## Usage

```
/reviewprs DevCallTopic
/reviewprs Copter
```

`$ARGUMENTS` is the label. Run it from the root of an ArduPilot checkout. Output
is written to `devcall_pr_reviews.html` in the **repository root**
(`git rev-parse --show-toplevel`), so it works in any ArduPilot clone. The report
has a review-dated table of contents with author + quick verdict, per-PR CI status,
changed-file links, file:line findings that deep-link into GitHub's diff view, and
a summary table; submodule/wiki PRs are prefixed with their repo (e.g.
`[ChibiOS] #123`, `[wiki] #7730`).

## Pipeline

1. **Find** every PR with the label across main + wiki + ArduPilot submodules,
   capturing each PR's current head commit.
2. **Fast incremental skip** — fetch the previously-published report for this label
   and compare head hashes (see below). Only new/changed PRs are actually reviewed.
3. **Review** the diff + CI of the new/changed PRs.
4. **Write** the HTML report (reused sections carried over verbatim + fresh ones).
5. **Codex validation** — Codex independently re-checks the newly-reviewed findings
   (CONFIRM / REFUTE / ADJUST / NEW); the report is revised on its own judgement,
   not blind acceptance, and a "Codex validation" line records the outcome.
6. **Post PR comments (`DevCallEU` label only)** — after validation, each reviewed
   PR with an actionable verdict (COMMENT / REQUEST CHANGES) gets its findings
   posted as a clearly AI-marked PR comment; existing AI comments are edited in
   place rather than duplicated. Other labels never post unless explicitly asked.
7. **Publish** to two places (see below).

## Incremental re-runs (head-hash skip)

Every PR section records the **head commit short hash** it was reviewed at, and the
report embeds a machine-readable manifest near the top:

```
<!-- reviewprs-manifest v1 label="DevCallTopic" generated="2026-06-09" heads="33372:69bff866c7 ..." -->
```

On a re-run the command fetches the per-label "latest" report, parses that manifest,
and for each currently-labelled PR:

- **reuse** if the head hash is unchanged — the prior section is copied over verbatim
  (it was already reviewed and Codex-validated at that exact commit), and it is *not*
  re-reviewed or re-validated;
- **review** if the PR is new or its head moved;
- **drop** if it merged / closed / lost the label.

So a re-run "to look for new/updated PRs" only pays for what actually changed — a
handful of PRs instead of the whole set.

## Publishing

The finished report is rsynced to the ArduPilot web host (`uav.tridgell.net`) in
two locations:

- per-label **latest** — `https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html`
  (canonical current report; the one re-runs read for the skip check);
- dated **archive** — `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html`.

`DevCallReviews/` has a `README.txt` and directory listing enabled. The reports are
public and **auto-generated — they may contain errors**; always confirm against the
actual PR before acting.

## Two behaviours worth knowing (learned the hard way)

- **Always re-fetches the current diff** for any PR it reviews. On a re-review it
  re-checks every prior finding against what's on the branch *now*, and calls out
  anything already fixed as "resolved since previous review" rather than repeating
  stale issues. (The head-hash skip only reuses a section when the head is byte-for-
  byte the same commit — so a reused section is by definition still current.)
- **Stamps the review date** prominently under the report title, and notes the
  refresh (added / changed / dropped / reused) on each pass — PR state changes daily,
  so reports need to be dateable and comparable.

## Adapting to a non-ArduPilot repo

The structure is generic; the ArduPilot specifics are: the wiki repo
(`ArduPilot/ardupilot_wiki`), the submodule sweep (owned by `ArduPilot/`), the
embedded-target review lens (flash/RAM concerns, coding patterns), and the
`uav.tridgell.net` publish host. Drop or retarget those and the rest (label filter →
head-hash skip → per-PR diff/CI review → Codex validation → HTML verdict report)
works for any GitHub project.
