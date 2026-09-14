#!/bin/bash
# Base clones of every repo the reviewprs workflow touches.
# ardupilot itself is cloned separately (it needs --recurse-submodules).
. "$HOME/review/bin/review-env.sh"
cd "$REVIEW_REPOS" || exit 1

REPOS="
ArduPilot/ardupilot_wiki
ArduPilot/MAVProxy
ArduPilot/pymavlink
ArduPilot/MissionPlanner
ArduPilot/SupportProxy
ArduPilot/useralerts
ArduPilot/CustomBuild
ArduPilot/MethodicConfigurator
ArduPilot/ArduRemoteID
ArduPilot/sphinx_rtd_theme
ArduPilot/WebTools
mavlink/mavlink
RsyncProject/rsync
"

for r in $REPOS; do
    d=$(basename "$r")
    # mavlink/mavlink and the ardupilot mavlink submodule share a basename;
    # key the upstream one distinctly so they never collide on disk.
    [ "$r" = "mavlink/mavlink" ] && d="upstream-mavlink"
    if [ -d "$d/.git" ]; then
        echo "=== $r: updating $d"
        ( cd "$d" && git fetch --all --prune -q && \
          git reset --hard "origin/$(git symbolic-ref --short HEAD 2>/dev/null || echo master)" -q ) \
          || echo "  WARN: update failed for $d"
    else
        echo "=== $r: cloning into $d"
        git clone -q "https://github.com/$r.git" "$d" || echo "  ERROR: clone failed for $r"
    fi
done
echo "REPOS_DONE"
