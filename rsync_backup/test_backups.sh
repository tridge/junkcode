#!/bin/sh
# check that all the backups are up to date

BACKUP_DIR=/backup
MAX_AGE=2
#NOTIFY_LIST="sysadmin@case.org.au"
NOTIFY_LIST="tridge@samba.org"

#################################################
# check one filesystem to see if it is up to date
check_filesystem() {
    fs="$1"
    if [ `find $fs/.backup_stamp -type f -ctime -$MAX_AGE | wc -l` != 1 ]; then
	cat <<EOF | mail -s "backup of $fs is out of date" $NOTIFY_LIST
WARNING!

This is a friendly warning from your backup robot. Your backup of 
$fs is more than $MAX_AGE days old. Please login to `hostname` and
have a look.

`ls -l $fs/.backup_stamp`
EOF
    fi
}

################
# check one server
check_server() {
    server="$1"
    for d in "$server"/*; do
	check_filesystem $d
    done
}

for s in $BACKUP_DIR/*; do
    check_server $s
done