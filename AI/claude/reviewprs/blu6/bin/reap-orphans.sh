#!/bin/bash
#
# Kill processes left behind by a review run.
#
# Review agents start test servers, SITL instances, ffmpeg, pytest suites and
# the like inside $REVIEW_DATA. When a run ends they are supposed to be gone,
# but they are not always: on 2026-09-09 a hung pytest from a finished run had
# been spinning a full core for 5h24m, plus two ffmpeg and five supportproxy
# processes idle for a day and a half.
#
#   reap-orphans.sh              # only if no run is active (safe for cron)
#   reap-orphans.sh --from-run   # called from a run's own exit trap
#
# A process is a candidate only if its cwd, exe or argv references $REVIEW_DATA.
# The user's own inspection tools are never touched.
set -u
. "$HOME/review/bin/review-env.sh" 2>/dev/null || exit 0
LOCK="$REVIEW_ROOT/etc/reviewprs.lock"
FROM_RUN=0
[ "${1:-}" = "--from-run" ] && FROM_RUN=1

# Outside a run's exit trap, refuse to reap while a run holds the lock - its
# children are legitimate work, not orphans.
# Test for an actual run process, not merely a held lock: pause-runs.sh also
# holds the lock, and a pause is precisely when reaping is safest.
if [ "$FROM_RUN" -eq 0 ]; then
    if pgrep -f "run-reviewprs\.sh" >/dev/null 2>&1; then
        echo "reap: a run is active; leaving its processes alone"
        exit 0
    fi
fi

# Never kill these even if they mention the review tree - they are how a human
# looks at a run in progress.
SAFE_RE='^(tail|less|more|vim|vi|emacs|nano|grep|cat|watch|man|sshd|bash|sh|zsh)$'

ME=$$
candidates=""
for d in /proc/[0-9]*; do
    p="${d#/proc/}"
    [ "$p" = "$ME" ] && continue
    # skip our own ancestry so the trap cannot kill the run that is invoking it
    case " $(ps -o ppid= -p $ME 2>/dev/null) " in *" $p "*) continue;; esac
    comm=$(cat "$d/comm" 2>/dev/null) || continue
    echo "$comm" | grep -qE "$SAFE_RE" && continue
    hit=0
    cwd=$(readlink "$d/cwd" 2>/dev/null); case "$cwd" in "$REVIEW_DATA"*) hit=1;; esac
    exe=$(readlink "$d/exe" 2>/dev/null); case "$exe" in "$REVIEW_DATA"*) hit=1;; esac
    if [ $hit -eq 0 ]; then
        args=$(tr '\0' ' ' < "$d/cmdline" 2>/dev/null)
        case "$args" in *"$REVIEW_DATA"*) hit=1;; esac
    fi
    [ $hit -eq 1 ] && candidates="$candidates $p"
done

[ -z "$candidates" ] && { echo "reap: nothing to reap"; exit 0; }

for p in $candidates; do
    args=$(tr '\0' ' ' < /proc/$p/cmdline 2>/dev/null | cut -c1-90)
    et=$(ps -o etime= -p $p 2>/dev/null | tr -d ' ')
    echo "reap: TERM pid=$p age=$et  $args"
    kill -TERM "$p" 2>/dev/null
done
sleep 5
for p in $candidates; do
    if kill -0 "$p" 2>/dev/null; then
        echo "reap: KILL pid=$p (ignored TERM)"
        kill -KILL "$p" 2>/dev/null
    fi
done
exit 0
