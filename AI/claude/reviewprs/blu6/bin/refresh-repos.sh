#!/bin/bash
# Keep the base clones current so each review starts from a fresh master.
# Serialised against review runs on the same lock - a base checkout must not
# move under a run that is using it via --shared/--reference.
set -u
. "$HOME/review/bin/review-env.sh"
LOG="$REVIEW_LOGS/refresh-repos-$(date +%Y%m%d_%H%M%S).log"
find "$REVIEW_LOGS" -name 'refresh-repos-*.log' -mtime +14 -delete 2>/dev/null
exec >>"$LOG" 2>&1
echo "refresh-repos start=$(date -Is)"
exec 9>"$REVIEW_ROOT/etc/reviewprs.lock"
if ! flock -n 9; then
    echo "SKIPPED: a review run holds the lock; base clones left alone"
    exit 0
fi
"$HOME/review/bin/clone-ardupilot.sh"
"$HOME/review/bin/clone-repos.sh"
echo "refresh-repos finish=$(date -Is) rc=$?"
