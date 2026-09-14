#!/bin/bash
#
# Cron entry point for the reviewprs workflow on blu6.
#
#   run-reviewprs.sh               -> /reviewprs          (the four-label sweep)
#   run-reviewprs.sh all           -> /reviewprs          (same thing, explicit)
#   run-reviewprs.sh followup      -> /reviewprs followup
#   run-reviewprs.sh rsync         -> /reviewprs rsync
#   run-reviewprs.sh AIReview      -> /reviewprs AIReview  (any label)
#
# No argument means the all-labels sweep, matching the skill's own convention
# that a bare /reviewprs is the four-label run. Do not change this to default to
# followup: the two differ by hours of work and the wrong one is silently plausible.
#
# All runs are serialised on one lock: the three label sub-runs share
# devcall_pr_reviews.html, and followup reads what they publish, so two
# concurrent runs would corrupt each other's output.
#
# The log is line-buffered and streamed, so `tail -f` is useful mid-run.

set -u
MODE="${1:-all}"

# The skill accepts `rsync`, `RSYNC`, `--rsync` and `/rsync` as the same mode, so
# the runner must too: `run-reviewprs.sh RSYNC` used to miss the account guard
# below and run tridge's own project on the ArduPilot subscription. Only the
# reserved words are folded - label names like AIReview are case-sensitive.
MODE="${MODE#--}"; MODE="${MODE#/}"
case "$(printf %s "$MODE" | tr "[:upper:]" "[:lower:]")" in
    rsync)    MODE=rsync ;;
    followup) MODE=followup ;;
    all)      MODE=all ;;
esac

. "$HOME/review/bin/review-env.sh"

# ArduPilot's venv carries pymavlink, empy, pexpect etc.
[ -f "$HOME/venv-ardupilot/bin/activate" ] && . "$HOME/venv-ardupilot/bin/activate"

# Per-mode Claude account, chosen before anything can spend quota: the EXIT trap
# below takes a closing usage reading, and on a lock-skipped rsync run that would
# otherwise be charged to - and recorded against - the wrong account.
if [ "$MODE" = "rsync" ] && [ -n "${REVIEW_RSYNC_CLAUDE_DIR:-}" ]; then
    export CLAUDE_CONFIG_DIR="$REVIEW_RSYNC_CLAUDE_DIR"
fi
CLAUDE_DIR="${CLAUDE_CONFIG_DIR:-$HOME/.claude}"

STAMP=$(date +%Y%m%d_%H%M%S)
LOG="$REVIEW_LOGS/reviewprs-${MODE}-${STAMP}.log"
LATEST="$REVIEW_LOGS/latest-${MODE}.log"
LOCK="$REVIEW_ROOT/etc/reviewprs.lock"
mkdir -p "$REVIEW_LOGS" "$REVIEW_ROOT/etc"

# Keep 30 days of logs; they are the only record of an unattended run.
find "$REVIEW_LOGS" -name 'reviewprs-*.log' -mtime +30 -delete 2>/dev/null

# A stable name to tail, always pointing at the newest run of this mode.
ln -sfn "$LOG" "$LATEST"

exec >>"$LOG" 2>&1
echo "=============================================================="
echo "reviewprs mode=$MODE  host=$(hostname)  start=$(date -Is)"
echo "REVIEW_DATA=$REVIEW_DATA  TMPDIR=$TMPDIR"
echo "follow with:  tail -f $LATEST"
echo "=============================================================="

# Lock policy. REVIEWPRS_LOCK_WAIT is seconds to wait for the lock:
#   0 (default)  - skip this slot if busy. Right for the frequent cron jobs,
#                  where the next slot is along soon anyway.
#   >0           - wait up to that long, then give up. Right for an on-demand
#                  run you actually want to happen, and for once-a-day jobs
#                  that would otherwise never run at all if they lose the race.
WAIT="${REVIEWPRS_LOCK_WAIT:-0}"
# Keep runs.html current no matter how this run ends - completed, skipped or
# failed. A skipped slot is exactly the kind of thing the page should show.
# On the way out: reap anything the run left running under $REVIEW_DATA, take a
# closing usage reading, then refresh the dashboard. The reaper runs first and
# its output goes to the run log, so a leak is visible where you would look for
# it rather than discovered days later by the fans.
LOCKED=0
trap 'FR=""; [ "$LOCKED" = 1 ] && FR="--from-run"; \
      "$HOME/review/bin/reap-orphans.sh" $FR 2>&1; \
      "$HOME/review/bin/claude-usage-probe.sh" end >/dev/null 2>&1; \
      "$HOME/review/bin/publish-runs-page.sh" >/dev/null 2>&1 || true' EXIT

exec 9>"$LOCK"
if [ "$WAIT" -gt 0 ]; then
    echo "waiting up to ${WAIT}s for the run lock..."
    if ! flock -w "$WAIT" 9; then
        echo "GAVE UP: lock still held after ${WAIT}s"
        echo "finish=$(date -Is) status=lock-timeout"
        exit 1
    fi
    echo "lock acquired at $(date -Is)"
else
    if ! flock -n 9; then
        echo "SKIPPED: another reviewprs run holds $LOCK"
        echo "finish=$(date -Is) status=skipped-locked"
        exit 0
    fi
fi
echo $$ >&9
LOCKED=1

case "$MODE" in
    all) PROMPT="/reviewprs" ;;
    *)   PROMPT="/reviewprs $MODE" ;;
esac

cd "$REVIEW_ROOT/work" || { echo "FATAL: no $REVIEW_ROOT/work"; exit 1; }

clear_stale_oauth_lock "$CLAUDE_DIR"

# Which account is this? A run on the wrong one is the failure that matters:
# it works, and quietly spends the wrong subscription. Refuse instead.
ACCOUNT=$(claude auth status --json 2>/dev/null | python3 -c '
import json,sys
try:
    d=json.load(sys.stdin)
except Exception:
    sys.exit(0)
print(d.get("email","") if d.get("loggedIn") else "")
' 2>/dev/null)
WANT=""
if [ "$MODE" = "rsync" ]; then
    WANT="${REVIEW_RSYNC_CLAUDE_ACCOUNT:-}"
    # An unset account must stop the run, not quietly fall through to the
    # default one: a missing etc/account.conf would otherwise put tridge's own
    # project back on the ArduPilot subscription with no sign in the log.
    if [ -z "$WANT" ]; then
        echo "FATAL: mode=rsync needs REVIEW_RSYNC_CLAUDE_ACCOUNT (set it in $REVIEW_ROOT/etc/account.conf)"
        echo "finish=$(date -Is) status=wrong-claude-account"
        exit 1
    fi
fi
if [ -n "$WANT" ] && [ "$ACCOUNT" != "$WANT" ]; then
    echo "FATAL: mode=$MODE must run as $WANT, but $CLAUDE_DIR is ${ACCOUNT:-not logged in}"
    echo "       log in once with:"
    echo "         CLAUDE_CONFIG_DIR=$CLAUDE_DIR claude auth login --email $WANT"
    echo "finish=$(date -Is) status=wrong-claude-account"
    exit 1
fi
echo "claude account: ${ACCOUNT:-unknown}  (config $CLAUDE_DIR)"

# Pre-flight: refuse to run if the permission rules are not in force. This box
# runs unattended, so a settings.json that has lost its deny list must stop the
# run, not silently grant it. "bypassPermissions" is deliberately NOT used
# below: it bypasses deny rules too, which would re-enable git push.
SETTINGS="$CLAUDE_DIR/settings.json"
if ! python3 - "$SETTINGS" <<'PYCHK'
import json,sys
try:
    d=json.load(open(sys.argv[1]))
except Exception as e:
    print("FATAL: cannot read settings.json: %s" % e); sys.exit(1)
deny=d.get("permissions",{}).get("deny",[])
need=["Bash(git push)","Bash(git push:*)"]
missing=[r for r in need if r not in deny]
if missing:
    print("FATAL: settings.json is missing deny rules: %s" % missing); sys.exit(1)
if d.get("permissions",{}).get("defaultMode")!="auto":
    print("FATAL: permissions.defaultMode is not 'auto'"); sys.exit(1)
print("permission pre-flight OK: git push denied, defaultMode=auto")
PYCHK
then
    echo "ABORTING: permission pre-flight failed"
    exit 1
fi

# gh is what actually gates a run: unauthenticated it is 60 requests/hour and
# the funnel alone needs more than that, so fail fast and clearly.
if ! gh auth status >/dev/null 2>&1; then
    echo "FATAL: gh is not authenticated. Run 'gh auth login' on blu6."
    echo "finish=$(date -Is) status=no-gh-auth"
    exit 1
fi
echo "gh pre-flight OK: $(gh auth status 2>&1 | sed -n 's/.*Logged in to [^ ]* account \([^ ]*\).*/\1/p' | head -1)"

# Bracket the run with real usage readings so the dashboard can show what this
# run itself consumed, rather than inferring it from token arithmetic.
# Quota pre-flight. Between 2026-09-11 01:47 and 2026-09-12 06:13, seventeen
# consecutive runs started and died one second later on "You've hit your weekly
# limit". They were harmless - nothing was written, posted or published - but
# they looked like plain rc=1 failures, so ~30h of silence took a human noticing
# that reviews had gone quiet. Check the meter first and say so plainly instead.
QMSG=$("$HOME/review/bin/claude-usage-probe.sh" start 2>&1)
QRC=$?
if [ "$QRC" -ne 2 ] && [ -n "$QMSG" ]; then
    echo "usage probe: $QMSG"           # non-fatal, but never silent
fi
if [ "$QRC" -eq 2 ]; then
    echo "SKIPPED: Claude quota exhausted -- ${QMSG:-weekly limit reached}"
    echo "finish=$(date -Is) status=quota-exhausted"
    exit 0
fi

# Publish immediately so the dashboard shows this run as soon as it starts.
# Without this a run is invisible until it ends or the periodic refresh fires,
# which is exactly the window in which someone asks "what is it doing?".
"$HOME/review/bin/publish-runs-page.sh" >/dev/null 2>&1 || true

START=$(date +%s)
# stream-json + the formatter gives one flushed line per event, so the log is
# monitorable while the run is in flight rather than only at the end.
# 9>&- closes the lock fd for claude and everything it spawns. Without it every
# descendant inherits fd 9, so a single orphaned child - and these runs do leave
# stray sleep timers and codex agents - keeps the lock held long after the run
# exits, making the *next* run skip for no reason. That silently cost the 07:47
# followup and the 18:13 all run on 2026-09-07.
# Pin the model explicitly rather than via the 'opus' alias, so a future alias
# change cannot silently move these runs onto a different model (and a different
# quota pool). High effort: these reviews are the whole point of the box.
stdbuf -oL -eL claude -p "$PROMPT" \
    --model claude-opus-5 \
    --effort high \
    --permission-mode auto \
    --add-dir "$REVIEW_ROOT" \
    --output-format stream-json \
    --verbose 9>&- \
  | stdbuf -oL python3 "$REVIEW_ROOT/bin/fmt-stream.py" 9>&-
RC=${PIPESTATUS[0]}
END=$(date +%s)

echo
echo "=============================================================="
echo "reviewprs mode=$MODE rc=$RC elapsed=$(( (END-START)/60 ))m finish=$(date -Is)"
echo "=============================================================="
exit $RC
