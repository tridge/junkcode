#!/bin/bash
# Base build: proves the toolchain works and warms ccache so review builds are fast.
# SITL is what reviews actually build; CubeOrange proves the ARM toolchain.
set -x
. "$HOME/review/bin/review-env.sh"
[ -f "$HOME/venv-ardupilot/bin/activate" ] && . "$HOME/venv-ardupilot/bin/activate"
cd "$REVIEW_REPOS/ardupilot" || exit 1

ccache -M 20G
ccache -z

./waf distclean

echo "########## SITL ##########"
./waf configure --board sitl        || { echo "SITL CONFIGURE FAILED"; exit 1; }
./waf copter plane rover sub -j"$(nproc)" || { echo "SITL BUILD FAILED"; exit 1; }

echo "########## CubeOrange (ChibiOS / ARM) ##########"
./waf configure --board CubeOrange  || { echo "CUBEORANGE CONFIGURE FAILED"; exit 1; }
./waf copter -j"$(nproc)"           || { echo "CUBEORANGE BUILD FAILED"; exit 1; }

# leave the tree configured for SITL, which is the common case for reviews
./waf configure --board sitl

ccache -s | head -12
echo "BASE_BUILD_DONE rc=0"
