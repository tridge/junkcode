#!/bin/sh

count=`find /home/tridge/backup/susan -type f -ctime -3 | wc -l`

if [ "$count" -lt 1 ]; then
    date | Mail -s 'susan backup failed' tridge
fi