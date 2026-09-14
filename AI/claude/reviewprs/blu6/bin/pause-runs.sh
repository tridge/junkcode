#!/bin/bash
# Hold the run lock for N minutes so no review run can start. Scheduled runs hit
# the lock and either skip or wait, exactly as they do behind a real run - no
# crontab edits, nothing to remember to restore, and it expires on its own.
#
#   pause-runs.sh 60      # pause for 60 minutes
#   pause-runs.sh status
#   pause-runs.sh resume  # end the pause early
set -u
. "$HOME/review/bin/review-env.sh" 2>/dev/null || exit 1
LOCK="$REVIEW_ROOT/etc/reviewprs.lock"
PIDF="$REVIEW_ROOT/etc/pause.pid"
INFO="$REVIEW_ROOT/etc/pause.info"

case "${1:-status}" in
  status)
    if [ -f "$PIDF" ] && kill -0 "$(cat "$PIDF")" 2>/dev/null; then
        echo "PAUSED: $(cat "$INFO" 2>/dev/null)"
        echo "  holder pid $(cat "$PIDF")   end early with: pause-runs.sh resume"
    else
        echo "not paused"; rm -f "$PIDF" "$INFO"
    fi ;;
  resume)
    if [ -f "$PIDF" ] && kill -0 "$(cat "$PIDF")" 2>/dev/null; then
        kill "$(cat "$PIDF")" 2>/dev/null && echo "pause ended; runs may start again"
    else
        echo "not paused"
    fi
    rm -f "$PIDF" "$INFO" ;;
  *)
    MINS="$1"
    case "$MINS" in ''|*[!0-9]*) echo "usage: pause-runs.sh <minutes>|status|resume"; exit 1;; esac
    if [ -f "$PIDF" ] && kill -0 "$(cat "$PIDF")" 2>/dev/null; then
        echo "already paused: $(cat "$INFO" 2>/dev/null)"; exit 0
    fi
    if ! flock -n "$LOCK" true 2>/dev/null; then
        echo "WARNING: a run currently holds the lock - it will finish first."
        echo "         This pause will take effect when it does."
    fi
    UNTIL=$(date -d "+$MINS minutes" '+%H:%M on %a %d %b')
    setsid nohup bash -c "exec 9>\"$LOCK\"; flock 9; echo \$\$ > \"$PIDF\"; sleep $((MINS*60))" \
        >/dev/null 2>&1 &
    sleep 1
    echo "paused until $UNTIL ($MINS min)" > "$INFO"
    echo "runs paused until $UNTIL"
    echo "  end early with: ~/review/bin/pause-runs.sh resume" ;;
esac
