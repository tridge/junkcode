#!/bin/bash

export MPLBACKEND=Agg
export PATH=$HOME/.local/bin:$HOME/bin:$PATH

cd ~/ardupilot-logs
(
 date
 /usr/bin/find all -name '*.tmp*' -mtime +1 -exec rm -f '{}' \;
 timelimit 3600 ../tools/analyse_logs.py all/*.BIN
 timelimit 3600 ../tools/create_maps.py all/*.BIN
 timelimit 3600 ../tools/generate_parms.py all/*.BIN
 timelimit 3600 ../tools/generate_msgs.py all/*.BIN
 timelimit 3600 ../tools/graph_logs.py all/*.BIN
 timelimit 3600 ../tools/log_summary.py --csv flights.csv
) >> run.log 2>&1
