#!/bin/bash
# Base clone of ArduPilot + submodules into the review repositories dir.
set -x
. "$HOME/review/bin/review-env.sh"
cd "$REVIEW_REPOS"
if [ ! -d ardupilot/.git ]; then
    git clone https://github.com/ArduPilot/ardupilot.git ardupilot || exit 1
fi
cd ardupilot
git remote set-url origin https://github.com/ArduPilot/ardupilot.git
git fetch --all --prune || exit 1
git checkout master && git reset --hard origin/master
git submodule update --init --recursive --jobs 4 || exit 1
echo "CLONE_DONE rc=$?"
