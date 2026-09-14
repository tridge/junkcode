#!/bin/bash
# Regenerate runs.html and publish it. Safe to call from anywhere, including
# from a run's exit path - it must never fail the run that invoked it.
. "$HOME/review/bin/review-env.sh" 2>/dev/null || exit 0
OUT="$REVIEW_DATA/runs.html"
python3 "$REVIEW_ROOT/bin/make-runs-page.py" "$OUT" >/dev/null 2>&1 || exit 0
rsync -a $RSYNC_AUTH "$OUT" "$REVIEW_PUBLISH/DevCallReviews/runs.html" >/dev/null 2>&1
exit 0
