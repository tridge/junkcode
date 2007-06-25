#!/bin/sh

export MAGIC_SCRIPT=/etc/magic/mymagic.sh

epxort LD_PRELOAD=/etc/magic/preload_open.so

myapp "$*"

