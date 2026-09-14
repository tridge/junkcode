#!/bin/bash
# Run ArduPilot's own prereq installer. DO_PYTHON_VENV_ENV=0 gives us the venv
# at ~/venv-ardupilot without appending an activate line to the shell profile.
set -x
cd "$HOME/review/repositories/ardupilot" || exit 1
export DO_PYTHON_VENV_ENV=0
export DEBIAN_FRONTEND=noninteractive
./Tools/environment_install/install-prereqs-ubuntu.sh -y
echo "PREREQS_DONE rc=$?"
