#!/bin/bash

# script to switch virtual audio between desktop and headset, linked to
# function keys on keyboard. The work is done in vmic.py

exec python3 "$(dirname "$(readlink -f "$0")")/vmic.py" "$@"
