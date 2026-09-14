#!/bin/bash
# Run a command with its own network namespace, so parallel SITL work does not
# have to move off TCP to stay out of its neighbours' way.
#
#   netns-run.sh <cmd> [args...]                   one-shot private namespace
#   netns-run.sh --session <dir> <cmd> [args...]   create-or-join <dir>'s namespace
#   netns-run.sh --session-stop <dir>              drop that namespace
#
# Why not --uds: AF_UNIX changes SITL's own behaviour, so a test can pass or
# fail differently from what the PR author sees. UARTDriver's anti-lag throttle
# is the clearest case - get_system_outqueue_limit() allows 65536 bytes of
# kernel outqueue on a unix socket against 1024 on TCP
# (libraries/AP_HAL_SITL/UARTDriver.cpp). --uds was only ever there to keep
# parallel runs off each other's ports; a private netns does that without
# touching the transport, because each run gets its own 127.0.0.1 and can use
# the default ports (5760, 5762, 5763).
#
# WRAP THE TEST, NOT THE AGENT. lo is the only interface in here: there is no
# route off the box and no DNS. An agent started inside it cannot reach its own
# model API - `curl https://api.openai.com` fails with "Could not resolve host"
# - so a Codex or Claude worker must run OUTSIDE the namespace and put this
# wrapper in front of the autotest/SITL commands it runs. Same for git fetch,
# pip install and terrain downloads: do them before, not inside.
#
# One-shot suits a whole autotest.py invocation, which starts and stops SITL
# itself. Use --session when an agent runs SITL in one command and talks to it
# from the next: those must land in the same namespace, and a fresh one-shot
# namespace per command cannot see the previous command's sockets.
#
# Ports are only half of it. autotest.py also serialises on $BUILDLOGS/autotest.lck
# and defaults BUILDLOGS to <repo>/../buildlogs, so two checkouts sharing a parent
# block on one lock however well their networks are separated. Give each agent a
# two-level root and put the checkout one level down - <agent>/wt - and that path
# is private by construction. This script deliberately does not set BUILDLOGS: one
# visible rule beats a second hidden one, and "autotest is locked" is a loud failure.
#
# Inside the namespace the caller is uid 0 mapped to the real user outside, so
# files stay owned by that user. Session holders are parked as
# netns-holder:<dir>, so reap-orphans.sh clears them with the rest of a run.
set -u

die() { echo "netns-run: $*" >&2; exit 1; }
warn() { echo "netns-run: $*" >&2; }

usage() { sed -n '3,6p' "$0" | sed 's/^# \?//'; exit 2; }

my_ns() { readlink /proc/self/ns/net; }

# A holder is valid only if the pid is alive AND still in the namespace it
# published. Without the recorded id, a recycled pid would hand the caller an
# unrelated namespace, or make --session-stop kill an innocent process.
holder_ok() {
    local pid=$1 want=$2 have
    [ -n "$pid" ] && [ -n "$want" ] || return 1
    have=$(readlink "/proc/$pid/ns/net" 2>/dev/null) || return 1
    [ "$have" = "$want" ]
}

read_session() {           # sets PID and NS from the session dir
    PID=$(cat "$1/.netns.pid" 2>/dev/null || true)
    NS=$(cat "$1/.netns.id" 2>/dev/null || true)
}

case "${1:-}" in
-h|--help|"") usage ;;

--session-stop)
    [ $# -eq 2 ] || die "usage: netns-run.sh --session-stop <dir>"
    SESSION=$2
    [ -d "$SESSION" ] || exit 0        # nothing was ever started here
    # Same lock as create-or-join: without it, stop can kill the old holder,
    # miss a replacement created in the gap, and then delete the replacement's
    # pid file - leaving a live holder nobody can find or stop.
    exec 8>"$SESSION/.netns.lock"
    flock 8
    read_session "$SESSION"
    if holder_ok "$PID" "$NS"; then
        kill "$PID" 2>/dev/null && echo "netns-run: stopped session $SESSION (pid $PID)"
    fi
    rm -f "$SESSION/.netns.pid" "$SESSION/.netns.id"
    ;;

--session)
    [ $# -ge 3 ] || die "usage: netns-run.sh --session <dir> <cmd> [args...]"
    SESSION=$2; shift 2
    mkdir -p "$SESSION" || die "cannot create $SESSION"

    # Already inside this session's namespace (a nested call): just run. The
    # alternative - treating our own namespace as "not the holder's" - would
    # spawn a second holder and split the agent's own commands in two.
    read_session "$SESSION"
    if [ -n "$NS" ] && [ "$NS" = "$(my_ns)" ]; then
        exec "$@"
    fi

    # Serialise create-or-join: several commands of one agent can start at once,
    # and two holders would mean two namespaces under one session directory.
    exec 8>"$SESSION/.netns.lock"
    flock 8
    read_session "$SESSION"
    if ! holder_ok "$PID" "$NS"; then
        # A pid file that no longer resolves means the holder died. Any SITL it
        # was hosting is now in a namespace we cannot rejoin, so say so loudly
        # rather than hand back a fresh empty network that silently cannot see it.
        [ -n "$PID" ] && warn "session $SESSION lost its namespace (holder $PID gone); starting a new one - anything still running in the old one is unreachable"
        rm -f "$SESSION/.netns.pid" "$SESSION/.netns.id"
        # 8>&- so the holder does not inherit the creation lock: if this shell is
        # killed before it unlocks, an inherited fd would keep the lock held for
        # as long as the holder sleeps, wedging every later call.
        setsid unshare --user --net --map-root-user bash -c '
            ip link set lo up || exit 1
            readlink /proc/self/ns/net > "$1.id.tmp" && mv "$1.id.tmp" "$1.id"
            echo $$ > "$1.pid"
            exec -a "netns-holder:$2" sleep 2147483647' _ "$SESSION/.netns" "$SESSION" \
            8>&- >/dev/null 2>&1 &
        for _ in $(seq 1 100); do
            read_session "$SESSION"
            holder_ok "$PID" "$NS" && break
            sleep 0.1
        done
    fi
    flock -u 8
    holder_ok "$PID" "$NS" || die "could not start a namespace for $SESSION"

    exec nsenter --preserve-credentials --user --net --target "$PID" -- "$@"
    ;;

*)
    exec unshare --user --net --map-root-user -- \
         bash -c 'ip link set lo up || { echo "netns-run: lo failed" >&2; exit 1; }; exec "$@"' \
         -- "$@"
    ;;
esac
