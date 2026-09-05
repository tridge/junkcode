# Review PRs by Label, Author, or Follow-up

Review a set of GitHub PRs and generate an HTML report. Checks the main ArduPilot repo, the ArduPilot wiki repo, all ArduPilot-owned submodule repos, the standalone ArduPilot repos (`SupportProxy`, `pymavlink`, `useralerts`, `MissionPlanner`, `MAVProxy`, `CustomBuild`, `MethodicConfigurator`, `ArduRemoteID`, `sphinx_rtd_theme` — the full list is in step 1), and the upstream `mavlink/mavlink` repo.

Run this from the root of an ArduPilot checkout (it reads `.gitmodules` in the working directory). The report is written to the repository root and works in any ArduPilot checkout, not just one.

The command is incremental: it records the head commit hash each PR was reviewed at, and on a re-run it reuses the previously-published report for any PR whose head is unchanged, only re-reviewing PRs that are new or have changed. This makes a re-run to pick up new/updated PRs very fast.

> **Scratch space:** all temporary checkouts, clones, tarballs and build trees for this
> command go under `/data/review/` (e.g. `mktemp -d -p /data/review pr34225-XXXX`). Never `/tmp` —
> it is a 46G tmpfs on this machine and filling it crashes X. Remove the dir when the review is done.
>
> **No partial clones.** Never `git clone --filter=blob:none` (or any other `--filter=`) here, and
> never run `git grep <sha>`, `git log -S`, or `git show <sha>:<path>` against a clone made that way.
> Every missing blob triggers a lazy `git fetch` subprocess, and `git grep` defaults to one thread per
> core, so a single grep spawns dozens of concurrent fetches that each write a promisor pack. On
> 2026-09-05 that turned one validate agent into ~2500 processes, load 630, and 46 GB of packs, and
> systemd-oomd killed the whole terminal cgroup — emacs included. Instead: `gh pr diff` for the diff,
> and if a real tree is needed, `git clone --shared --no-checkout <existing /data/review checkout>` (or
> `--reference` against one). Those are local, cost nothing, and fetch no blobs.
>
> **This rule goes into every Codex/agent task prompt verbatim**, alongside the `/tmp` rule — the
> agents are the ones that make the clones, and they will reach for `--filter=blob:none` by habit.

## Unattended runs — the default

**This command must never block on a question.** It is run on a schedule (session cron today,
system cron later), so a prompt that waits for a human is not a pause: it is a run that did
nothing, reported nothing, and will not be noticed until someone looks. A scheduled run that
stops half way is strictly worse than one that never started, because the labels look swept.

**Unattended is the default posture.** Every decision point that used to ask now has a
documented default, takes it, and says so in the report and the summary. Pass `--interactive`
explicitly — as a separate word in the argument — to restore the asking behaviour. Nothing else
enables it: a bare `/reviewprs`, `/reviewprs followup`, or any cron-fired invocation is
unattended.

Four rules make a run non-blocking. They override any "ask the user" wording later in this file.

1. **Never end the turn mid-run.** Emitting user-facing prose ends the turn, and in a scheduled
   context there is no next message to resume from — the run simply stops, usually right after a
   tidy-looking progress summary, which is the most misleading possible place to die. So produce
   **exactly one** user-facing message per run, at the very end, after step 9 has published.
   Everything before that goes into tool calls: write progress to `$SCRATCH/progress.log` if it
   is worth recording. Waiting on Codex is not an exception — set the monitor and keep working on
   whatever does not depend on it. "I'll continue in a moment" is a stop.

2. **A batch too large to review properly is deferred, never asked about.** Count the REVIEW set.
   If it exceeds what can be done at full depth, review **oldest-first** up to what can, and defer
   the rest. Deferral is explicit, not silent — it must satisfy all four of:
   - name every deferred PR, with its head, in the report and in the final summary;
   - carry each deferred PR's **previous section over verbatim**, clearly marked
     `DEFERRED — not re-reviewed this run`, so the report never implies a review that did not
     happen (a newly-labelled PR with no previous section is listed as deferred and unreviewed);
   - **keep its old manifest head** (or omit it entirely if it is new). This is what makes
     deferral self-healing: the next scheduled run compares the current head against that stale
     entry, still sees a difference, and retries the PR automatically. Writing the current head
     for a PR you did not review is the one thing that turns a deferral into a permanent skip;
   - never substitute a shallower method for the deferred PRs — no Codex-only pass, no skim.
     Depth is fixed; the *number of PRs* is what flexes.

3. **Upstream `mavlink/mavlink` comments are held, never waited on.** Review the PR and include it
   in the report exactly as normal, then **skip posting** and record it as
   `comment held — upstream repo, needs approval`, naming it in the summary. Do not wait for a
   yes. The user can approve them in one message afterwards; a held comment costs a day, a hung
   run costs the whole sweep.

4. **A failure in one part does not abort the rest.** This already applies to the four sub-runs of
   an all-labels run; it applies equally to a single PR whose diff will not fetch, a Codex agent
   that returns nothing, or a publish that fails. Record it, carry on, and name it in the summary.
   The only condition that legitimately stops a run before it starts is an argument that resolves
   to no mode at all (step 5 below) — under cron the argument is fixed and known-good, so that
   should never fire.

**What still reaches the user.** None of this makes the run quieter about substance. Deferrals,
held comments, failures and skips are all named explicitly in the one closing message. The rule
being removed is "stop and wait", not "tell the user".

## Arguments

**Strip `--interactive` first, before anything else.** It is a posture flag, not a mode selector: remove
it from the argument, remember that the run is interactive, and resolve what remains by the rules below.
An argument of `--interactive` alone is therefore an interactive **all-labels run**, not an unresolvable
mode. Without it the run is unattended (see **Unattended runs** above), which is the default.

`$ARGUMENTS` selects which PRs to review. **With no argument at all** it is the **all-labels run**: a
`DevCallTopic`, `DevCallEU`, `AIReview` and `followup` sweep back to back, producing up to four sets of
review web pages plus PR comments (see resolution step 0 below). Otherwise the argument picks one of four **modes**:

- **LABEL mode** — a GitHub label (e.g. `DevCallTopic`, `Copter`, `Plane`). Reviews every open PR
  carrying that label. Most labels produce a report only; the labels that **auto-post** comments are
  `DevCallEU`, `DevCallTopic`, and **`AIReview`** (the last is the same opted-in signal ArduPilot shares
  with the rsync repo — see step 8).
- **AUTHOR mode** — a GitHub username, optionally written `@name`. Reviews every **open** PR by that
  author that was **updated in the last 7 days**.
- **FOLLOWUP mode** — the literal word `followup`. Reviews every PR that this workflow has already
  reviewed and commented on, which is **still open** and whose code has **changed since that comment**.
  Its purpose is to shorten the loop for a developer who has pushed changes to address an AI review and
  is waiting to hear whether they landed — so unlike the other modes it is designed to be run often and
  to do nothing at all when nothing has moved.
- **RSYNC mode** — the literal word `rsync`. A **completely separate** review target from all ArduPilot
  work: it reviews the open PRs labelled **`AIReview`** on the **`RsyncProject/rsync`** repo (the rsync
  file-transfer tool, a C codebase — **not** ArduPilot, so ArduPilot house rules do not apply). It has
  its own single-page report at `https://uav.tridgell.net/RsyncReviews/index.html` and its own manifest,
  and — like the dev-call labels — it **auto-posts** comments (this is tridge's own project). It is
  incremental and re-runnable exactly like LABEL mode: a re-run re-reviews only the PRs whose head has
  changed (or that are newly labelled), reusing the rest, and posts/updates comments with the same
  deprecate-and-repost rules. See the RSYNC-specific notes in steps 1, 3, 8 and 9.

**Resolve the mode first and say which one you picked**, before anything else — discovery, the publish
path and the comment policy all differ:

0. **No argument at all — the ALL-LABELS RUN.** If `$ARGUMENTS` is empty or only whitespace, this invocation
   is shorthand for four back-to-back runs: **`DevCallTopic` (LABEL), then `DevCallEU` (LABEL), then
   `AIReview` (LABEL), then `followup` (FOLLOWUP)**, in exactly that order. Run them **sequentially, as four
   complete independent runs** — each does its own full step 1-9 (discovery, incremental skip, review +
   Codex, report, comment posting, publish) under its own mode's rules, exactly as if invoked with that
   argument on its own. Do **not** try to merge them into one report or run them in parallel: the three
   label runs share the same local file (`devcall_pr_reviews.html`) so cannot overlap, and — the reason for
   the order — **`followup` must run last**, because it discovers its set from the *published* per-label
   reports and the *posted* comments, so it needs the three label runs to have already published and
   commented at the current heads. Consequences of that ordering, all intended:
   - The three label runs each publish their per-label latest **and** a dated archive
     (`DevCallTopic` → upcoming Tuesday, `DevCallEU` → upcoming Wednesday, both Canberra time; `AIReview`
     → today, since it has no associated dev call) and each auto-posts comments (all three are
     comment-posting labels — step 8). Each sweeps **all repos** (main, wiki, the ArduPilot submodules and
     the standalone ArduPilot repos — SupportProxy, pymavlink, useralerts, MissionPlanner, MAVProxy, CustomBuild,
     MethodicConfigurator, ArduRemoteID — plus, for the report only, upstream `mavlink/mavlink`).
   - `followup` then reads those fresh reports; every PR the three label runs just re-reviewed is now at its
     told-head with a current comment, so `followup` correctly **skips** it. `followup` therefore acts
     only on PRs from *other* published label reports whose code moved since their last comment — often a
     small set or none. That is the design working, not a bug: it is why this is "**up to** 4 sets of
     review web pages" — if nothing else has moved, `followup` publishes and posts nothing and that fourth
     set is simply absent.
   - Publish paths never collide: `DevCallTopic/`, `DevCallEU/`, `AIReview/`, and `followups/<DATE_TIME>/`
     are four distinct destinations, so the four runs' web pages coexist.
   - Report a **combined summary** at the end — one clearly-labelled block per sub-run (mode, skip split,
     verdict counts, comments posted, published URL), plus the `followup` funnel line. If a sub-run fails,
     say so and continue with the others rather than aborting the whole run; never let a later
     sub-run's result silently overwrite an earlier one's summary.

   This empty-argument case is checked **before** everything below. A bare `/reviewprs` is never AUTHOR
   mode on an empty username or any other guess.

1. `followup` (case-insensitive, with or without a leading `/` or `--`) is a **reserved word** and wins
   over everything else. Check it before the label/user tests. This matters: there is no `followup` label on
   ArduPilot/ardupilot, but **`FollowUp` is a real GitHub username**, so without this step the argument
   resolves to AUTHOR mode and sweeps a stranger's PRs. If someone genuinely wants that user, `@FollowUp`
   forces AUTHOR mode.
1a. `rsync` (case-insensitive, with or without a leading `/` or `--`) is likewise a **reserved word** →
   RSYNC mode. Check it here, before the label/user tests: `rsync` is neither an ArduPilot label nor the
   intended GitHub user, and RSYNC mode is a wholly separate target (repo `RsyncProject/rsync`, label
   `AIReview`, report `RsyncReviews/index.html`). It does **not** touch any ArduPilot repo or report. If
   someone genuinely wants a GitHub user named `rsync`, `@rsync` forces AUTHOR mode.
2. A leading `@` forces AUTHOR mode; strip it and use the rest as the username.
3. Otherwise test it as a label on the main repo and look for an exact, case-insensitive match:
   `gh label list --repo ArduPilot/ardupilot --search "$ARGUMENTS" --json name --jq '.[].name'`.
   Match → LABEL mode.
4. Otherwise test it as a user: `gh api users/<arg> --jq .login`. Resolves → AUTHOR mode.
5. If neither resolves, **stop and say so**. Do not guess: a mistyped label would otherwise sweep zero
   PRs and publish a confidently empty report. This is the **only** legitimate halt in the whole workflow,
   and it happens before any work is done — everything after this point runs to completion under the
   **Unattended runs** rules. Under a schedule the argument is fixed and known-good, so it should never
   fire; if it does, the cron entry itself is wrong and needs fixing.

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

| | LABEL mode | AUTHOR mode | FOLLOWUP mode | RSYNC mode |
|---|---|---|---|---|
| local file | `devcall_pr_reviews.html` | `user_pr_reviews_<user>.html` | `devcall_pr_reviews.html`, rebuilt once per affected label | `rsync_pr_reviews.html` |
| published "latest" | `https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html` | `https://uav.tridgell.net/UserReviews/<USERNAME>.html` | `https://uav.tridgell.net/DevCallReviews/followups/<DATE_TIME>/devcall_pr_reviews.html` (one report per run), **plus** a refresh of every per-label report containing a re-reviewed PR | `https://uav.tridgell.net/RsyncReviews/index.html` (a single living page — destination is the filename `index.html`, not a directory) |
| dated archive | `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html` | none | n/a — the run's own report **is** the dated one | none — the single page is simply kept current |
| re-run reads | the per-label latest | the per-user page | every per-label latest, plus the posted comments | `RsyncReviews/index.html` |

`<DATE>` is the date of the **upcoming dev call**, not the day the review ran — the upcoming Tuesday for
`DevCallTopic`, the upcoming Wednesday for `DevCallEU`, both in Canberra time, with "upcoming" including
today. That is what lets a review be published days before the meeting and still land where people will
look on the day. Any other label uses today. Full rule and the traps in step 9, "Dated archive: use the
call date, in Canberra time".

The "latest" URL for the active mode is the one a re-run fetches to decide what to skip (step 2), and the
one step 9 publishes to. AUTHOR mode has no dated archive — the per-user page is simply kept current.
FOLLOWUP mode does not own a report of its own: it updates the existing per-label reports in place, so
that their manifests stay truthful and a later LABEL run does not redo the same work.

1. Find the PRs to review. **Capture each PR's current head commit** (`headRefOid`); the short hash is its first 10 characters.

   **LABEL mode** — every open PR carrying the label, across the main repo, the wiki repo and submodules
   (the per-repo commands are listed below).

   **RSYNC mode** — every **open** PR carrying the `AIReview` label on **`RsyncProject/rsync`** only. There
   are no submodules, wiki or upstream repos to sweep — it is a single repo:
   ```bash
   gh pr list --repo RsyncProject/rsync --label "AIReview" --state open \
              --json number,title,author,url,updatedAt,headRefOid --limit 100
   ```
   Key each PR by its bare number (all one repo, no collision). This set — open PRs with the `AIReview`
   label — is exactly the "current reviews" the single `RsyncReviews/index.html` page shows: a PR that is
   closed/merged or has the label removed is **DROPPED** from the page on the next run (step 2), and a PR
   whose head moved is **re-reviewed** and its comment updated (step 8). Nothing about the ArduPilot
   discovery below (`.gitmodules`, wiki, `mavlink`, `SupportProxy`) applies in RSYNC mode.

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

   **FOLLOWUP mode** — the set is derived from what has already been reviewed and commented on, not from
   a label or an author. Build it like this:

   1. **List the published reports.** `curl -fsS https://uav.tridgell.net/DevCallReviews/` returns a
      browsable index. Take the directory entries that are **not** dated archives and **not** this mode's
      own output — i.e. drop anything matching `^[0-9]{4}_[0-9]{2}_[0-9]{2}` (which also correctly drops
      oddities like `2026_07_15_DevCallTopic/`), drop **`followups/`**, and drop `README.txt`:
      ```bash
      curl -fsS https://uav.tridgell.net/DevCallReviews/ \
        | grep -oE 'href="[^"]+/"' | sed 's/href="//;s#/"##' \
        | grep -vE '^([0-9]{4}_[0-9]{2}_[0-9]{2}|followups|\.|/|\?)' | sort -u
      ```
      **`followups` must be excluded explicitly** — it does *not* match the dated pattern, so without its
      own rule it is treated as a label, and fetching `followups/devcall_pr_reviews.html` 404s (it is a
      parent directory holding per-run subdirectories, not a report). A 404 there is silent under
      `curl -fsS`, so the run would simply see one fewer label and skip whatever only that label covered.
      What remains is the set of per-label "latest" reports, e.g. `DevCallEU/`, `DevCallTopic/`. Fetch each
      one's `devcall_pr_reviews.html` and parse its manifest comment.
   2. **Union the manifest keys into one candidate set**, remembering *every* label a PR appears under —
      a PR commonly appears in more than one, and step 9 has to update all of them.
   3. **Get the last-told head from the posted comment, not from the manifest.** This is the crux of the
      mode and getting it wrong produces duplicate reviews. The manifests disagree with each other by
      design, because each label was last run on a different day: as of 2026-08-19, `#34094` sits at
      `a6537afc71` in `DevCallEU` but `db80aa4121` in `DevCallTopic`, and `#34073` and `#33933` differ the
      same way. Keying off a manifest would therefore re-review a PR that another label reviewed at the
      current head only hours earlier, and post a second comment saying nothing new. What actually matters
      is **what the developer was last told**, which is the head quoted in the newest AI comment on the PR:
      ```bash
      ME=$(gh api user --jq .login)
      LAST=$(gh api --paginate repos/<owner>/<repo>/issues/<n>/comments \
        --jq "[.[] | select(.user.login==\"$ME\") | select(.body|test(\"AI-generated\"))] | last | .body")
      TOLD=$(sed -nE 's/.*head `([0-9a-f]{10})`.*/\1/p' <<<"$LAST" | head -1)
      ```
      **Extract it with `sed`, not `grep -oE 'head \`...'`.** A backslash-backtick inside a
      `grep` pattern is a GNU extension meaning *start of buffer*, not a literal backtick, so under GNU
      grep that pattern matches nothing and `TOLD` comes back empty — every candidate then falls out as
      "no head in the comment" and the mode reports a clean, empty, completely wrong sweep. It is easy to
      miss because it depends on which `grep` is on PATH: verified 2026-09-02 that this machine's
      interactive `grep` is a ugrep wrapper, under which the bad pattern works, while `/usr/bin/grep`
      (GNU 3.12) returns no match for the same input. **A cron run gets the GNU one**, so the bug is
      invisible in an interactive test and fatal in the scheduled run it matters for. `sed -nE` sidesteps
      it entirely and yields the hash in one step. If you prefer grep, put the backtick in a bracket
      class instead of escaping it — that form is literal under both greps.

      A PR with **no** such comment is not a follow-up case at all — nobody has been given feedback to
      respond to — so drop it and say how many you dropped for that reason.
   4. **Filter to the actual work.** Keep a PR only if all of these hold, and report the count that fails
      each test rather than silently narrowing:
      - `state == OPEN` (skip merged/closed; note them, since a merged PR with an unaddressed finding may
        deserve a follow-up issue instead — see `#34107` on 2026-08-19 for exactly that case);
      - it is not a draft (a draft is still being worked on; note it and move on);
      - `current head != TOLD`.
   5. **Drop no-op head moves before reviewing, not after — but do it by comparing the PR's own patch at
      each head, NOT with the compare API.** Head movement is often a rebase or a merge from master with no
      change to the author's own work, and re-reviewing those burns budget while telling the developer
      nothing. The correct test:
      ```bash
      git fetch -q origin <TOLD> && git fetch -q origin pull/<n>/head
      OLD=<TOLD>; NEW=<current>
      BO=$(git merge-base origin/master $OLD);   BN=$(git merge-base origin/master $NEW)
      gh pr view <n> --repo <owner/repo> --json files --jq '.files[].path' | sort > own.txt
      mapfile -t OWN < own.txt

      # 1. TEXT delta, immune to line shifts.  Strip the hunk headers and the
      #    index blob line: master changing lines ABOVE the PR's hunks moves the
      #    @@ offsets and the blob ids without the author touching anything.
      norm() { sed -E 's/^@@ -[0-9,]+ \+[0-9,]+ @@.*/@@/; /^index [0-9a-f]+\.\.[0-9a-f]+/d'; }
      git diff $BO $OLD -- "${OWN[@]}" | norm > old.patch
      git diff $BN $NEW -- "${OWN[@]}" | norm > new.patch
      cmp -s old.patch new.patch && TEXT_SAME=yes || TEXT_SAME=no

      # 2. BINARY delta, which step 1 cannot see: git renders a binary as
      #    "Binary files ... differ" with NO content, so once the index line is
      #    stripped the patch is identical however much the blob changed.
      #    Compare the author's contribution directly - the (merge-base blob ->
      #    head blob) pair at each head.
      blob() { git rev-parse --quiet --verify "$1:$2" 2>/dev/null || echo -; }
      BIN_SAME=yes; BIN_WHY=
      for f in "${OWN[@]}"; do
          [ "$(git diff --numstat $BN $NEW -- "$f" | cut -f1)" = - ] || continue  # text: done above
          ob=$(blob $BO "$f"); oh=$(blob $OLD "$f")
          nb=$(blob $BN "$f"); nh=$(blob $NEW "$f")
          [ "$oh" = "$nh" ] && continue
          BIN_SAME=no
          if   [ "$ob" = - ] && [ "$nb" = - ]; then BIN_WHY="$BIN_WHY $f(added,regenerated)"
          elif [ "$ob" = "$nb" ];             then BIN_WHY="$BIN_WHY $f(modified,base-stable)"
          else                                     BIN_WHY="$BIN_WHY $f(modified,base-moved)"
          fi
      done

      if [ "$TEXT_SAME" = yes ] && [ "$BIN_SAME" = yes ]; then
          echo REBASE-ONLY
      else
          R=REAL-CHANGE
          [ "$TEXT_SAME" = no ] && R="$R text"
          [ -n "$BIN_WHY" ]     && R="$R binary:$BIN_WHY"
          echo "$R"
      fi
      ```
      Both dimensions unchanged ⇒ rebase-only: **skip it, do not post, and list it in the summary as
      "moved but unchanged"** so the skip is visible rather than looking like an oversight.

      **Normalise the hunk headers, or every rebase reads as a real change.** A rebase onto a master that
      touched lines *above* the PR's own hunks shifts every `@@ -a,b +c,d @@` offset and every `index
      <old>..<new>` line, while the author's content is byte-identical — so a raw `diff -q` of the two
      patches reports REAL-CHANGE and the PR gets a full pointless re-review. Verified on `#28530` on
      2026-09-02: the entire difference between the two patches was one `index` line and
      `@@ -8114` → `@@ -8116`. Because a rebase-only skip deliberately posts no comment, the told-head
      never advances, so such a PR resurfaces on *every* subsequent follow-up run — misclassifying it is
      therefore a permanent cost, not a one-off.

      **But do not stop at the normalised text compare, because it is blind to binaries.** Git prints
      `Binary files a/x and b/x differ` with no content, so the `index <old>..<new>` line is the *only*
      textual evidence a binary changed — and step 1 has just deleted it. The two fixes pull in opposite
      directions and both are needed: normalising is what makes the text test correct, and it is exactly
      what destroys the binary signal. Verified on `#34117` on 2026-09-02, where a 105 KB `.hex` bootloader
      had been regenerated between heads: the normalised patches are identical, so text alone says
      REBASE-ONLY and the run would have skipped a PR whose committed firmware image had changed.

      The blob comparison must distinguish **added** from **modified** files, because a changed head blob
      means different things in each case. For a file the PR *adds*, the author's contribution is the whole
      file, so a different head blob is unambiguously new content. For a file the PR *modifies*, the head
      blob also moves whenever master edits that file under a rebase — so compare the merge-base blob too:
      base identical and head moved means the author changed it, while base moved as well is ambiguous and
      should be treated as REAL-CHANGE (reviewing an unchanged PR costs budget; skipping a changed one
      costs the author their answer).

      **Do not use `gh api .../compare/<old>...<new>` for this.** It is a *three-dot* compare, so its base
      is `merge-base(old, new)` — and on a **force-pushed or rebased branch that is the branch point, so it
      returns the entire PR** and any file-overlap test is trivially 100%. Verified on `#34094` on
      2026-08-20: `compare` reported `status: diverged, ahead=86, behind=12`, 128 files of which all 37 of
      the PR's own files "changed" — while the true delta was 42 lines. Restricting to the PR's own files
      does not save you either, for the same reason. Force-pushes are the common case in this mode, since
      an author responding to review usually amends rather than appends.

      Two further traps the patch-diff method also avoids: the merge-base moves between the two heads, so
      files that master changed in between (on `#34094`, `Tools/ros2/*` and a workflow file) show up as
      spurious differences unless you restrict to the PR's own file list; and a rebase inflates raw counts
      enormously — on `#31355`, `compare` reported 114 changed files of which only 21 belonged to the PR,
      so sizing a review off that number would wildly overestimate the work.
   6. Give each surviving PR the same **key** as the other modes (`<number>`, or `<reponame>#<number>`),
      and record which labels' reports must be updated for it.

   **The funnel is heavily weighted towards skipping, and that is the design working.** Measured on
   2026-08-20 against the two published label reports: 27 unique candidates → 6 merged, 13 open but
   unchanged, 0 without a comment → **8 re-reviewed**. Of the 7 PRs present under both labels, 3 had
   disagreeing manifest heads (`#33933`, `#34073`, `#34094`); keying off a manifest would have re-reviewed
   all three, whereas the posted-comment head correctly skipped two of them as already current. Expect the
   great majority of candidates to fall out at the state/unchanged tests, and treat a run where most
   candidates survive as a signal that something is wrong with the head extraction rather than as a real
   backlog.

   The step 3 rule about batch size applies here too: 8 PRs needing a full double review is a real batch,
   not a quick check. Count the survivors before starting and, if it is more than can be done properly,
   review oldest-first and defer the remainder per **Unattended runs** rule 2 — oldest-first is especially
   right in this mode, since those authors have been waiting longest. Deferral is cheap here: a deferred
   follow-up keeps its old told-head, so the very next run of this mode picks it up again.

   If the surviving set is empty, that is the expected and healthy outcome for a frequently-run mode: say
   plainly that nothing has moved since the last review, publish nothing, post nothing, and stop.

   **Re-check the SKIPPED-as-unchanged set at the end of the run, not only the set you are posting to.**
   A follow-up run takes 30-60 minutes, and an author who is actively responding to review is exactly the
   person likely to push during it — so the skip decision made at discovery can be stale by the time the
   run finishes. The step-8 head re-check covers only the PRs being commented on and will not catch this.
   Before writing the summary, re-fetch `headRefOid` for every PR you skipped as unchanged and compare it
   against the head you recorded at discovery:
   ```bash
   for n in $(cut -d'|' -f1 "$SCRATCH/skipped_unchanged.txt"); do
     cur=$(gh pr view $n --repo <owner/repo> --json headRefOid --jq '.headRefOid[0:10]')
     was=$(grep "^$n|" "$SCRATCH/skipped_unchanged.txt" | cut -d'|' -f2)
     [ "$cur" = "$was" ] || echo "MOVED DURING RUN: $n ($was -> $cur)"
   done
   ```
   Any output is a decision: either fold that PR into this run (usually right if the run has not finished
   publishing) or **name it explicitly in the summary as deferred to the next run**, with the new head, so
   it is visible rather than silently absent. Never let it fall out unmentioned.

   This is not hypothetical. On 2026-08-20 `#34094` was correctly skipped at discovery (23:53Z), its author
   force-pushed at 00:04:57Z — twelve minutes later, mid-run — and that push *removed the watchdog pats*
   the previous review had flagged, i.e. it was a direct response to the feedback. The run finished and
   published without mentioning the PR at all, which is the one outcome this mode exists to prevent.

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
   - Upstream MAVLink: `gh pr list --repo mavlink/mavlink --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - SupportProxy: `gh pr list --repo ArduPilot/SupportProxy --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50` — an ArduPilot-owned standalone repo that is **not** a submodule, so the `.gitmodules` sweep below will not find it; key its PRs `SupportProxy#<number>`. It is an ArduPilot repo, so it is treated like the main/wiki/submodule repos (the `mavlink/mavlink` upstream exceptions do **not** apply): comment-posting in step 8 happens normally for DevCallEU/DevCallTopic (and for the `AIReview` label).
   - Other ArduPilot-owned standalone repos — also **not** submodules, so the `.gitmodules` sweep below will not find them; **sweep each one explicitly** with the same command shape (`gh pr list --repo ArduPilot/<repo> --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`) and key its PRs `<reponame>#<number>`:
     - `ArduPilot/pymavlink`
     - `ArduPilot/useralerts`
     - `ArduPilot/MissionPlanner`
     - `ArduPilot/MAVProxy`
     - `ArduPilot/CustomBuild`
     - `ArduPilot/MethodicConfigurator`
     - `ArduPilot/ArduRemoteID`
     - `ArduPilot/sphinx_rtd_theme`

     These are ArduPilot repos, so they are treated exactly like the main/wiki/submodule repos — the `mavlink/mavlink` upstream exceptions do **not** apply, and comment-posting in step 8 happens normally for `DevCallEU`/`DevCallTopic`/`AIReview`.

     `ArduPilot/sphinx_rtd_theme` needs the explicit sweep for a reason worth remembering: it is the
     wiki's Sphinx theme, and it is a submodule of **neither** `ArduPilot/ardupilot` nor
     `ArduPilot/ardupilot_wiki`, so no `.gitmodules` pass reaches it. Added 2026-09-05 after `#24` sat
     `AIReview`-labelled and was never picked up by the sweep — it had to be reviewed by hand twice.
     It is an ArduPilot-owned fork of the third-party `readthedocs/sphinx_rtd_theme`, so review it as
     an ArduPilot repo (post comments normally) but judge changes against **the fork's own
     conventions**, which are not ArduPilot's: its templates are Jinja, its boolean theme options go
     through the `|tobool` filter, and its only consumer is `ardupilot_wiki` — a theme change is
     usually paired with a wiki PR, so check that one too before calling a forward reference dangling. Two disambiguations: `ArduPilot/pymavlink` is a distinct repo from the nested `pymavlink` submodule (there is no key collision — the submodule sweep only reaches ardupilot's *top-level* submodules, and pymavlink is nested under `mavlink`), and `ArduPilot/mavlink` (the fork) is already swept via `.gitmodules` and keyed `mavlink#`, so do **not** add it here. Keep this list current: if ArduPilot adds another standalone repo that people put dev-call/AIReview labels on, add it here.
   - Parse `.gitmodules` to find all submodule URLs hosted under `ArduPilot/` or `ardupilot/` on GitHub
   - For each ArduPilot-owned submodule repo, run: `gh pr list --repo <owner/repo> --label "$ARGUMENTS" --json number,title,author,url,updatedAt,headRefOid --limit 50`
   - Combine all results, tracking which repo each PR belongs to. Give each PR a stable **key**: the PR number for the main repo, or `<reponame>#<number>` for wiki/submodule PRs (e.g. `mavlink#360`, `wiki#7730`). Note that wiki PRs are documentation-focused (ReST under `*/source/docs/`); review for technical accuracy vs the current ArduPilot codebase, broken `:ref:` cross-references, ReST syntax, and consistency with existing wiki conventions.
   - **Disambiguate the two mavlink repos.** `mavlink/mavlink` and `ArduPilot/mavlink` share the basename `mavlink`, and the fork is a submodule so it is swept as well — a bare `<reponame>` key is therefore ambiguous between them. Key the upstream repo `upstream-mavlink#<number>` and the fork `mavlink#<number>`, and label them `[upstream-mavlink]` and `[mavlink]` in the report. Without this, two different PRs collide on one manifest key and the incremental skip in step 2 silently reuses one PR's review for the other — the failure is invisible, because the report still looks complete. Apply the same rule to any future non-ArduPilot repo whose basename already exists in the submodule set.
   - **Upstream `mavlink/mavlink` is not an ArduPilot repo**, so weigh findings accordingly: it serves every MAVLink implementation, not just ArduPilot. A message or enum change there affects PX4, QGC, MAVSDK and pymavlink users too, XML dialect changes are effectively permanent once released, and ArduPilot conventions (parameter name limits, `new` returning zeroed memory, per-subsystem commits) do **not** apply. Review for dialect/XML correctness, backward compatibility of message and enum changes, and whether `common.xml` is the right place versus a vendor dialect. Do not tell an upstream author to follow an ArduPilot house rule.

2. **Fast incremental skip — decide what actually needs reviewing.**

   **In FOLLOWUP mode step 1 has already done this**, and by a stricter test (posted-comment head, plus
   the rebase check), so do not redo it here: every surviving PR is a REVIEW, there is no REUSE set to
   compute, and nothing is DROPPED. Skip to step 3. The one part of this step that still applies is the
   cheap CI refresh, which you need for the *other* PRs in each report you are about to rebuild — see
   step 9.

   For LABEL, AUTHOR and RSYNC modes, fetch the previously-published report and compare head hashes:
   - LABEL mode: `curl -fsS https://uav.tridgell.net/DevCallReviews/$ARGUMENTS/devcall_pr_reviews.html`
     AUTHOR mode: `curl -fsS https://uav.tridgell.net/UserReviews/<USERNAME>.html`
     RSYNC mode: `curl -fsS https://uav.tridgell.net/RsyncReviews/index.html`
     (a 404/failure means this is the first run — treat every PR as new and review all of them).
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

   **In FOLLOWUP mode the previous round's comment is the most important input to this step.** The whole
   point of the mode is that the developer has pushed changes in response to it, so the review is not a
   fresh read that happens to land on a moved PR — it is an answer to a specific question they are waiting
   on. Before writing anything, fetch your own previous comment (step 1 already located it) and the delta
   since:
   ```bash
   gh api repos/<owner>/<repo>/compare/<TOLD>...<current head> --jq '.files[] | "\(.status) \(.filename)"'
   ```
   Then go through **every finding in that previous comment** and classify it explicitly as **RESOLVED**
   (say how, referencing the change), **STILL OPEN** (say why the new code does not cover it, and mark it
   re-raised rather than presenting it as new), or **DISPUTED** (the author answered in the thread or in a
   commit message — engage with their reasoning on the merits; if they are right, say so plainly and drop
   the finding). A follow-up comment that silently omits a previous finding is the worst outcome here,
   because the developer cannot tell whether it was fixed, forgotten, or withdrawn. New findings on newly
   added code are of course still in scope.

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

   **When the batch is still genuinely too large to review properly, defer the excess — do not silently
   ration depth, and do not stop to ask.** Count the REVIEW set before starting. If even the parallel form
   will not cover it, review oldest-first up to what will, and defer the rest under the deferral rules in
   **Unattended runs** rule 2: named in the report and summary, previous section carried over and marked
   `DEFERRED`, old manifest head preserved so the next scheduled run retries it automatically.

   Quietly downgrading half the batch to a shallower method and disclosing it in the report afterwards is
   still wrong — depth per PR is fixed, and it is the *number* of PRs that flexes. What has changed is only
   that the run no longer halts for an answer: under a schedule that answer never comes, and the whole
   sweep is lost rather than the tail of it. In `--interactive` mode, and only there, ask instead: report
   the count and offer to split by label subset, by repo, or oldest-first.

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

   - **RSYNC mode reviews the rsync codebase, not ArduPilot — weigh findings accordingly.** `RsyncProject/rsync`
     is a portable C program (client/server file transfer over a wire protocol), so the relevant angles are
     different from an ArduPilot PR and the ArduPilot house rules (16-char params, `new`/`malloc` returning
     zeroed memory, per-subsystem commits) **do not apply**. Review for: memory safety in C (buffer
     overflows, off-by-one, unchecked `malloc`, use-after-free, integer overflow in size math — rsync has a
     CVE history here); **wire-protocol compatibility** (a change to the protocol must stay compatible with
     older peers or bump `PROTOCOL_VERSION` deliberately, and both sender and receiver sides must agree);
     correct handling of untrusted input from the remote peer (path traversal, filename sanitisation);
     signed/unsigned and 32-bit/64-bit portability across the many platforms rsync targets; and matching the
     project's own C style and existing idioms. As with upstream `mavlink`, review it on its own terms; do
     not tell an rsync author to follow an ArduPilot convention.

4. Write the HTML report directly to the report file (`devcall_pr_reviews.html` in the repository root; in RSYNC mode `rsync_pr_reviews.html`, which is published as `RsyncReviews/index.html`) — do not read the file first, it is pure generated output. Build it from the REUSE sections (copied verbatim from the fetched prior report) plus freshly-written sections for the REVIEW set:
   - As the first line inside `<body>`, emit the review manifest comment covering **every** PR in the final report (reused + reviewed): `<!-- reviewprs-manifest v1 label="$ARGUMENTS" generated="YYYY-MM-DD" heads="<key>:<shorthash> ..." -->`. The next run depends on this. **`generated` is the date the review actually ran**, not the call date the archive directory is named for (step 9) — it records when the heads were captured, which is exactly what the incremental skip in step 2 reasons about.
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
   - Include the absolute review date at the top of the report. For `DevCallTopic` and `DevCallEU`, name
     the call it is for as well, since the two are usually different days and the dated archive directory
     is keyed on the call — e.g. "Review date: **2026-08-24** · for the `DevCallTopic` call on
     **2026-08-25**". Do not silently print only one of them.
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
     SCOPE=codexpool-$$
     setsid nohup systemd-run --user --scope --quiet \
        -p MemoryMax=40G -p TasksMax=4000 -p CPUQuota=1600% \
        --unit="$SCOPE" bash -c "
        export SCRATCH_DIR=$SCRATCH
        cat $SCRATCH/todo.txt | xargs -P 6 -I{} bash $SCRATCH/worker.sh {}
        touch $SCRATCH/POOL_DONE
     " </dev/null > "$SCRATCH/pool.log" 2>&1 &
     disown 2>/dev/null

     # wait for it — poll the SENTINEL, never the process table.
     # A Bash tool call times out, so this is normally written as a BOUNDED loop and re-run;
     # the stop below must therefore be guarded on the sentinel, never run unconditionally
     # after the loop, or a bounded poll that simply ran out of time kills a healthy pool.
     for i in $(seq 40); do [ -f "$SCRATCH/POOL_DONE" ] && break; sleep 15; done

     # reap leftovers: codex exec's 30s command timeout leaves backgrounded work running,
     # so the sentinel does not mean the pool's descendants are gone
     [ -f "$SCRATCH/POOL_DONE" ] && systemctl --user stop "$SCOPE.scope" 2>/dev/null
     ```

     **The pool MUST run in its own transient systemd scope**, as above. `setsid` detaches it from the
     terminal but leaves it in the terminal's cgroup, so when a runaway agent drives memory up,
     systemd-oomd kills that whole cgroup — which on 2026-09-05 meant the gnome-terminal tab holding
     emacs and every Claude and Codex session in it. A `--user --scope` with `MemoryMax` makes the pool
     its own kill target: oomd takes the pool and nothing else. `TasksMax` caps a process-spawning
     runaway before it reaches load 600.

     **Size `TasksMax` in threads, not processes** — the cgroup `pids` controller counts tasks, so a
     browser-driving agent costs far more than the process list suggests. Measured 2026-09-05 on two
     Codex agents each driving headless Chrome: **431 tasks for only 43 processes**, about 10:1. An
     earlier `TasksMax=500` here throttled that perfectly normal run and had to be raised mid-flight.
     4000 leaves room for six such agents and still stops a 2500-*process* storm (which is well over
     10,000 tasks) long before it reaches load 600. `MemoryMax` is the primary guard; `TasksMax` is the
     backstop. If a legitimate run ever bumps it, raise it live with
     `systemctl --user set-property <unit>.scope TasksMax=<n>` rather than killing the pool.

     **`CPUQuota` is what keeps the desktop alive.** `MemoryMax`/`TasksMax` bound how big the runaway
     gets, but neither bounds CPU: on 2026-09-05 load reached 600 and X, emacs and the terminal became
     unusable before oomd fired. Cap the pool below the core count so the desktop always has cores left
     — `1600%` on this 24-core machine leaves 8. Size it as `(nproc - 8) * 100%`; the pool runs slower
     under a storm and unthrottled otherwise, since six agents rarely saturate 16 cores. Verify the
     scope after launch with
     `systemctl --user show "$SCOPE.scope" -p MemoryMax -p TasksMax -p CPUQuotaPerSecUSec`
     (`CPUQuota` reads back as `CPUQuotaPerSecUSec=16s`, not as a percentage). This needs the `cpu`
     controller delegated to the user manager — check
     `cat /sys/fs/cgroup/user.slice/user-$(id -u).slice/user@$(id -u).service/cgroup.controllers`
     lists `cpu memory pids`. If `systemd-run` is unavailable, fall back to
     plain `setsid nohup` but say so in the report, because the blast radius is then the whole terminal.

     Three traps, all of which cause a partial run that looks like a complete one:

     - **A backgrounded `xargs -P` pool dies with its launcher.** Started via `run_in_background`,
       the tool reported "completed" as soon as the wrapper returned, having launched only the first
       6 of 21 agents; the remaining 15 never ran. The logs that *did* exist looked fine, so the
       shortfall is invisible unless you count them. `setsid nohup` + `disown` survives this.
     - **`pgrep -x codex` is NOT a completion signal.** It matches every Codex process on the
       machine, including other Claude sessions', so it reports "still running" long after your own
       pool has died and "finished" is never reliable either. Poll the sentinel file instead.
     - **The sentinel does not mean the work stopped.** `codex exec` yields after ~30 s and does not
       reap what it backgrounded, so an agent that finishes can leave its commands running. On
       2026-09-05 the reviewers "finished" at 02:42 and the orphaned git storm ran until oomd fired at
       03:27. Always `systemctl --user stop "$SCOPE.scope"` after the sentinel, and check the scratch
       dir's size before declaring the run clean. **Guard that stop on the sentinel existing.** Hit on
       2026-09-05: the wait was written as a bounded `for` loop (a Bash tool call cannot block forever)
       with the stop after it, so when the loop ran out of time it killed two healthy Codex agents ten
       minutes into a browser-based check — logs truncated mid-run, no final answer, both had to be
       re-run. A bounded poll that expires means "not finished yet", not "finished".

     Then **count the logs against the input set before using any of them**, and re-run the
     stragglers — a missing agent is a PR that received no validation at all:
     ```bash
     for n in $(cat "$SCRATCH/todo.txt"); do [ -s "$SCRATCH/validate_$n.log" ] || echo "MISSING $n"; done
     ```

     Note `codex exec` logs interleave the agent's tool transcript with its prose, and the final
     answer is **not** reliably the tail of the file. Extract findings by grepping for the
     `BUG`/`ISSUE`/`NOTE` markers you asked for rather than by slicing the end of the log.

     Measured on 2026-07-29: three agents launched this way completed in **47 s** wall-clock, against roughly 3.5 min per PR when the same work was done serially in one agent. Verified that concurrent `codex exec` runs leave `~/.codex/.claude-threads/` byte-identical — no session state is written, so there is nothing to race on.

     Every task prompt — finding-validation and cold spot-check alike — must open with the scratch-space
     and **no-partial-clone** rules from the top of this file, quoted verbatim. Both exist because an
     agent broke them, and an agent that has not been told will reach for `/tmp` and
     `git clone --filter=blob:none` by default.

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

8. **Post review comments to the PRs — LABEL mode (DevCallEU/DevCallTopic/AIReview) and RSYNC mode.** This step runs automatically when the mode is LABEL and the label is exactly **`DevCallEU`**, **`DevCallTopic`**, or **`AIReview`**, **and in RSYNC mode** (`RsyncProject/rsync`, `AIReview` label — tridge's own project, opted in). The **`AIReview` label on an ArduPilot PR is the same opted-in signal as on the rsync repo** — someone applied it to request an auto-posted AI review — so it auto-posts exactly like the dev-call labels (same scope, AI-generated marker, and edit-vs-repost mechanics below); it simply has no associated dev call, so its dated archive uses today's date (step 9), not a call date. **Exception: PRs in `mavlink/mavlink` are never auto-commented** (this covers an `AIReview`-labelled upstream-mavlink PR too). That is a third-party upstream project rather than an ArduPilot one — a posted review there goes to maintainers who have not opted into this process and lands under the running user's name on someone else's repo. Review and include those PRs in the report exactly as normal, then **hold** the comment: record it in the report as `comment held — upstream repo, needs approval`, name it in the closing summary with the exact `gh pr comment` that would post it, and carry on. **Do not wait for a yes** — under a schedule nobody is there to give one, and blocking here throws away a completed sweep for a comment that can just as well go out a day later (**Unattended runs** rule 3). In `--interactive` mode you may ask instead. Everything else about the mechanics is unchanged. For every other label, do **not** post any comments unless the user explicitly asks you to in their message. (When they do ask for another label, follow the same mechanics below.)

   **RSYNC mode posts exactly like the dev-call labels, with two specifics.** (a) Post only **after both the
   Claude read (step 3) and the Codex pass (step 7) are complete and reconciled** — never on the strength of
   one pass alone (this is the same rule as everywhere, restated because it is what the user asked for). (b)
   Add `--repo RsyncProject/rsync` to every `gh`/`gh api` call, and link the comment to the single page
   `https://uav.tridgell.net/RsyncReviews/index.html#pr<number>`. The comment scope (comment on every
   reviewed PR, including clean APPROVEs), the AI-generated marker, and the **edit-in-place vs
   deprecate-and-repost** decision are all identical to LABEL mode: if your prior AI comment is still the
   last thing on the PR, edit it in place; if anything has been posted since, deprecate-and-repost so the
   update lands at the bottom and notifies. A re-run that finds a PR's head unchanged posts nothing (it is a
   REUSE); a re-run that finds the head moved re-reviews and updates the comment by that same test.

   **FOLLOWUP mode always posts, and always as a NEW comment — never an in-place edit.** Posting *is* the
   mode's purpose, and it needs no label check, because every PR in its set was selected precisely by
   already having one of our comments on it. The upstream `mavlink/mavlink` exception above still applies.

   **Skip the edit-vs-repost test in this mode and always deprecate-and-repost.** The test asks "would an
   in-place edit be buried?", which is the wrong question here. An in-place edit generates **no
   notification at all** — and in this mode the author has, by construction, just pushed changes in
   response to the previous comment and is waiting to hear whether they landed. Editing silently means
   they are told nothing, which defeats the entire purpose of the mode. Observed on 2026-08-20 on `#34094`:
   the author pushed ~670 lines answering nearly every finding but posted no comment alongside it, so the
   `NEWER == 0` branch said "edit in place" — i.e. respond to a developer actively waiting for feedback by
   silently rewriting a comment they had already read. The mechanics of deprecating the old comment and
   posting the new one are exactly as described below; only the choice is removed.

   In this mode the comment should **open with the verdict on the previous round**, not with the new
   findings — that is the thing the developer is waiting to read. Something like "Re-reviewed at head `X`:
   3 of the 4 findings from my previous comment are resolved, 1 is still open, details below." Then the
   RESOLVED / STILL OPEN / DISPUTED triage from step 3, then anything new. If every previous finding is
   resolved and nothing new turned up, say exactly that in a couple of lines and move the verdict — an
   author who has fixed everything should be told so unambiguously and promptly, which is the entire
   reason this mode exists.

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
   - **Link the comment to the report it came from**, on its own line near the top, so the author can see
     the full context and what else was checked. **The URL differs per mode — use the one for the mode you
     are actually running**, and construct it from the path you published to in step 9 rather than from
     memory:
     - LABEL mode → the **DATED archive**, never the per-label "latest":
       `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html#pr<key>`, where `<DATE>` is
       exactly the archive directory step 9 computes for this label (`DevCallTopic` → the upcoming Tuesday,
       `DevCallEU` → the upcoming Wednesday, both Canberra time; any other label → today).
       **Never link `DevCallReviews/<LABEL>/devcall_pr_reviews.html` from a comment.** That path is
       overwritten by every later run of the label, so an author or maintainer opening the link weeks
       afterwards lands on a different report and cannot find the review being referenced. The dated
       directory is the stable anchor, and for the dev-call labels it names the call the review was
       prepared for — exactly what someone looking back wants.
       Two consequences worth stating rather than hiding: the dated directory is the *current call week's*
       folder, so a later run in the same week rewrites it, and a PR that merges (or loses the label) is
       dropped from the set and loses its section there — if you are commenting on such a PR, say so in the
       comment instead of leaving a link to a section that is no longer present. And since the comment must
       contain a live URL, **publish (step 9) before posting (step 8) in LABEL mode too**, not just in
       FOLLOWUP/RSYNC.
     - AUTHOR mode → `https://uav.tridgell.net/UserReviews/<USERNAME>.html` (only relevant if the user
       explicitly asked for comments, since this mode does not post by default). That single page is
       deliberately kept current and has no dated archive, so it is the only available target.
     - FOLLOWUP mode → `https://uav.tridgell.net/DevCallReviews/followups/<DATE_TIME>/devcall_pr_reviews.html`
       — the run's own directory, **not** a per-label URL. A follow-up comment linking to a label report is
       wrong twice over: that report is a different document, and it will be overwritten by the next label
       run, so the link rots. This path is already dated, so it needs no further adjustment.
     - RSYNC mode → `https://uav.tridgell.net/RsyncReviews/index.html#pr<number>` — the single living page
       with the per-PR anchor; it has no dated archive, so it is the only available target.
     Because the comment must contain the published URL, publish (step 9) **before** posting (step 8) in
     **every** mode that links to a report — the two steps are in the reverse of their usual order there,
     since the comment links to the page you just published. Verify the URL returns 200, **and that the
     `#pr<key>` anchor is actually present in the published page**, before putting it in a comment that goes
     to an author.
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
     `NEWER > 0` ⇒ something came after it ⇒ **deprecate-and-repost**.
     `NEWER == 0` ⇒ nobody has spoken since ⇒ apply the head test below before concluding "edit".

     **A push counts too, and the comment count alone will not see it.** The three sources above are
     comments and reviews; a force-push or new commits are none of those, so an author who answered your
     review *in code* and said nothing leaves `NEWER == 0` — and an in-place edit generates no
     notification, so the one person actively waiting to hear is told nothing. Compare the head you are
     about to quote against the head your previous comment quoted:
     ```bash
     TOLD=$(jq -r .body <<<"$MINE" | sed -nE 's/.*head `([0-9a-f]{10})`.*/\1/p' | head -1)
     NOW=<the head this run reviewed>          # the same hash the new comment will open with
     if [ -n "$TOLD" ] && [ "$TOLD" != "$NOW" ]; then
         DECISION=repost      # the PR moved since we last told them
     elif [ "$NEWER" -gt 0 ]; then
         DECISION=repost      # somebody else spoke after us
     else
         DECISION=edit        # same head, nobody spoke - an edit is seen and a second comment is noise
     fi
     ```
     This is the same reasoning FOLLOWUP mode already applies unconditionally, and it belongs here for
     the same reason: that mode reaches these PRs only when it happens to run first, and a LABEL sweep
     catches exactly the same authors mid-response. Observed on `#33975` on 2026-09-02 — the author
     force-pushed 37 minutes after the comment, no one else posted, so the comment-only test said "edit"
     and the re-review landed silently on a developer who had just pushed. `#34094` on 2026-08-20 was the
     same shape with ~670 lines of response behind it.

     Note the head test only fires when there *is* a previous head to compare with. A PR newly labelled
     into this run, already carrying a comment from another label's sweep at the same head, has
     `TOLD == NOW` and correctly gets an edit — which is the case the original test was right about and
     which this does not disturb.

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

   **For the two dev-call labels the dated directory is the date of the CALL, not the date you ran the
   review** — see "Dated archive: use the call date, in Canberra time" below. That is what lets a report,
   and the PR comments linking to it, be published days ahead of the meeting and still land in the folder
   people will look in on the day.

   ```bash
   # LABEL mode
   REPORT="$(git rev-parse --show-toplevel)/devcall_pr_reviews.html"
   LABEL="$ARGUMENTS"

   # Dated archive directory: DevCallTopic -> upcoming Tuesday, DevCallEU -> upcoming Wednesday,
   # both reckoned in Canberra time. "Upcoming" includes today. Any other label -> today.
   case "$LABEL" in
       DevCallTopic) CALL_DAY=Tuesday ;;
       DevCallEU)    CALL_DAY=Wednesday ;;
       *)            CALL_DAY= ;;
   esac
   if [ -n "$CALL_DAY" ] && [ "$(TZ=Australia/Sydney date +%A)" != "$CALL_DAY" ]; then
       DATE=$(TZ=Australia/Sydney date -d "next $CALL_DAY" +%Y_%m_%d)
   else
       DATE=$(TZ=Australia/Sydney date +%Y_%m_%d)
   fi

   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$LABEL/   # per-label latest
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/$DATE/    # dated archive

   # AUTHOR mode — a single page per user, kept current; no dated archive.
   # Note the destination is a FILENAME, not a directory: the trailing path component
   # is <USERNAME>.html, so rsync must be given that exact target path.
   USER_NAME=<username>
   REPORT="$(git rev-parse --show-toplevel)/user_pr_reviews_${USER_NAME}.html"
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/UserReviews/${USER_NAME}.html

   # RSYNC mode — a single living page, kept current; no dated archive.
   # Like AUTHOR mode, the destination is a FILENAME (index.html), not a directory,
   # and it lives in its own RsyncReviews/ tree, entirely separate from DevCallReviews/.
   REPORT="$(git rev-parse --show-toplevel)/rsync_pr_reviews.html"
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/RsyncReviews/index.html
   ```

   **FOLLOWUP mode writes its own dated report under `followups/`, and additionally refreshes the label
   reports** so their manifests stay truthful:

   ```bash
   DATE_TIME=$(date +%Y_%m_%d_%H%M)     # e.g. 2026_08_20_1432 — one directory per run
   # 1. the run's own report — this is the URL the posted comments link to
   rsync -Pavz --mkpath "$REPORT" tridgell.net:UAV-web/DevCallReviews/followups/$DATE_TIME/
   # 2. for each label whose report contained a re-reviewed PR, refresh that label's latest
   rsync -Pavz --mkpath "$LABEL_REPORT" tridgell.net:UAV-web/DevCallReviews/$LABEL/
   ```

   The `followups/` report covers only the PRs re-reviewed in that run and is the run's deliverable. The
   per-label refresh is a separate, secondary write whose only job is to stop the label reports showing a
   stale review and re-triggering the same follow-up; it replaces just those PRs' sections and manifest
   heads and carries everything else over verbatim.

   - Rebuild each affected label's report by taking its **published** version, replacing only the sections
     for the PRs re-reviewed this run, and updating those PRs' entries in the manifest, the
     contents/quick-verdict table and the summary table. Every other section is carried over verbatim, and
     its CI status refreshed as in step 2 — the report must stay a complete, accurate picture of that
     label, not become a followup-only page.
   - **A PR that appears under several labels must be written into all of them at the same head.** That is
     what stops the manifests drifting apart again and re-triggering the same follow-up next run. Verify
     it before publishing: for each re-reviewed key, grep the new head out of every rebuilt report and
     confirm they agree.
   - Do **not** write the plain `<DATE>/` archive in this mode — that name belongs to full LABEL runs, and
     it is now a *call* date, so a follow-up report covering three PRs would masquerade as the complete
     sweep prepared for that dev call. `followups/<DATE_TIME>/` is deliberately a separate subtree, and is
     timestamped to the minute (from the local clock — it is a run stamp, not a call date) because this
     mode is expected to run several times a day.
   - If a re-reviewed PR is no longer in any label's report (it was reviewed once and then the label was
     removed), there is nothing to rebuild — the comment on the PR is the deliverable. Say so in the
     summary rather than inventing a report for it.

   - LABEL mode serves at `https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html`, with the dated snapshot at `https://uav.tridgell.net/DevCallReviews/<DATE>/devcall_pr_reviews.html` — where `<DATE>` is the call date computed above, so for the dev-call labels that URL is usually in the future when you print it. Print it anyway: it is the link to hand out ahead of the meeting.
   - AUTHOR mode serves at `https://uav.tridgell.net/UserReviews/<USERNAME>.html`. Because it is one page per user rather than a directory, publishing **replaces** the previous run's page — that is intended, since the 7-day window means an archive of author sweeps would mostly be duplicates. Print the URL when done, as for LABEL mode.
   - RSYNC mode serves at `https://uav.tridgell.net/RsyncReviews/index.html`. It is one living page listing every currently-open `AIReview` PR; publishing **replaces** it each run (with reused sections carried over verbatim), so it always reflects the current set — a PR that closed/merged or lost the label simply drops off. There is no dated archive. Print the URL when done.
   - `--mkpath` creates the destination directory if it doesn't exist (the `UAV-web/DevCallReviews/` and `UAV-web/RsyncReviews/` parents already exist, or `--mkpath` creates them). If a host's rsync predates 3.2.3 and lacks `--mkpath`, first `ssh tridgell.net mkdir -p UAV-web/<dir>`, then rsync without `--mkpath`.
   - Print both public URLs when done (in RSYNC mode, the single `RsyncReviews/index.html` URL).

   **Dated archive: use the call date, in Canberra time.** The two dev-call labels name real meetings with
   fixed weekdays, and the whole point of running the review early is that the report and the PR comments
   are already in place when people arrive. So `<DATE>` is the date of the **upcoming call**, not the date
   the review ran:

   | label | call | `<DATE>` |
   |---|---|---|
   | `DevCallTopic` | Tuesday morning Canberra time (09:00 AEST / 10:00 AEDT) | the upcoming **Tuesday** |
   | `DevCallEU` | Wednesday evening Canberra time (17:00 AEST / 18:00 AEDT) | the upcoming **Wednesday** |
   | anything else | no associated call | today |

   Four things about that computation, each of which is a way to get it wrong:

   - **"Upcoming" includes today.** If it is already Tuesday in Canberra, `DevCallTopic` uses *today's*
     date, not next week's. This is not what `date -d "next Tuesday"` does — GNU `date` skips to the
     following week when the base day already is that weekday (verified: base `2026-08-25` (Tue) gives
     `2026-09-01`). Hence the explicit same-day test in the snippet; do not simplify it away.
     Note the consequence at the far end of the day: a run late on Tuesday evening, after the Topic call
     has already happened, still writes that Tuesday's directory. That is deliberate — the rule is "the
     current call week", and a same-day re-run should update the day's report rather than start next
     week's.
   - **Canberra, not UTC and not the runner's clock.** Use `TZ=Australia/Sydney` explicitly (Canberra
     shares Sydney's rules; `Australia/Canberra` is an alias for the same zone). Canberra is UTC+10/+11, so
     for the whole local morning the UTC date is still *yesterday* — at 2026-08-23 22:52 UTC it was already
     Monday the 24th in Canberra. That is not a cosmetic difference: at Wednesday 09:00 Canberra (= Tuesday
     23:00 UTC), `DevCallTopic` computed in Canberra gives **2026_09_01** and computed in UTC gives
     **2026_08_25** — a full week out, and pointing at a Tuesday that has already gone.
   - **DST needs no special handling** *for the date*, which is the only thing this affects. The clock time
     of each call shifts by an hour across the AEST/AEDT boundary (first Sunday in October, first Sunday
     in April) but the weekday does not, and the tz database applies the shift for you. The hours are
     recorded in the table above only so the report can state them; nothing computes from them.
   - **The dated directory and the report's own review date now differ, and that is correct.** The report
     header carries the date the review was actually performed (step 4: "Include the absolute review date
     at the top of the report"), while the directory carries the call it is for. When they differ, say so
     in the report header — e.g. "Reviewed 2026-08-24 · for the DevCallTopic call on 2026-08-25" — so a
     reader who notices the mismatch is not left wondering which one is stale. The manifest's
     `generated="..."` attribute stays the **review** date; it records when the heads were captured, which
     is what step 2's incremental skip reasons about.

   The per-label "latest" directory is unaffected by any of this — it is always overwritten with the newest
   report regardless of which call it was built for.

Report the summary counts when complete, in **one message, at the end of the run** (**Unattended runs**
rule 1). Say which **mode** was used and what selected the set — the label, or the username plus the
`updatedAt` cutoff that produced it. Note the skip split and validation outcome (including which APPROVE
PRs were spot-checked cold), any PR comments posted/updated, and give the published URL.

**Four things must never be omitted, because each is a place where work silently did not happen:**
- **Deferred PRs** — every one named with its head, and the note that the next scheduled run will retry it.
- **Held comments** — every upstream `mavlink/mavlink` PR whose comment was withheld, with the `gh pr
  comment` line that would post it, so approving them is a copy-paste rather than a re-run.
- **Failures** — any PR whose diff would not fetch, Codex agent that returned nothing, or publish that
  did not land.
- **Moved-during-run PRs** — from the end-of-run re-check, whether folded in or deferred.

If all four are empty, say so in a few words rather than leaving the reader to infer it from silence.
The point of a scheduled run is that nobody watched it happen, so the summary is the only evidence of
what it did and did not do.

X APPROVE | Y COMMENT | Z REQUEST CHANGES (reused N, reviewed M; Codex: X confirmed / Y adjusted /
Z refuted / W added, K APPROVE spot-checked / J reclassified; comments posted on P PRs) — published at
`https://uav.tridgell.net/DevCallReviews/<LABEL>/devcall_pr_reviews.html` (LABEL mode),
`https://uav.tridgell.net/UserReviews/<USERNAME>.html` (AUTHOR mode), or
`https://uav.tridgell.net/RsyncReviews/index.html` (RSYNC mode).

In AUTHOR mode, if the sweep returns **no** PRs, say that plainly — "no open PRs by `<user>` updated
since `<cutoff>`" — and do not publish an empty page over a previously useful one.

In RSYNC mode, report the same split as LABEL mode (reviewed / reused / dropped), name the RSYNC repo and
`AIReview` label as what selected the set, and give the single `RsyncReviews/index.html` URL. If there are
**no** open `AIReview` PRs, say so plainly and do not overwrite a previously useful page with an empty one.

In FOLLOWUP mode, report the funnel rather than just the outcome, because the interesting number is
usually how much was correctly skipped:

    N candidates from M label reports → S still open with one of our comments → T heads moved →
    U re-reviewed (V skipped as rebase-only) — P comments posted; W findings resolved since last round,
    X still open, Y new. Reports updated: <label list>.

If nothing has moved, say "no reviewed PR has changed since its last review" and stop — publishing and
posting are both skipped. That is a successful run, not an empty one, and it should be cheap enough to
run on a schedule.
