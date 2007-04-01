#!/bin/sh
# a incremental rsync backup script


# config information
LOCKFILE=/var/lock/backup.lock
LOGFILE=/var/log/backup.log
EXCLUDE_FILE=/etc/backup_excludes
NUM_INCREMENTAL=3
BACKUP_DIR=/backup
RSYNC_OPTS="--bwlimit=500 --exclude-from=$EXCLUDE_FILE --ignore-errors --delete --delete-excluded -axv --backup --timeout=3600"



##################################
# support functions


##################################
# backup one directory
backup_one() {
    path="$1"
    name="$2"
    DAY=`date +%j`
    DAY=`expr $DAY % $NUM_INCREMENTAL`
    IDIR=$BACKUP_DIR/incremental/$name/$DAY

    rm -rf $IDIR
    mkdir -p $IDIR

    mkdir -p $BACKUP_DIR/$name
    date
    echo Backing up $name to $path and $IDIR
    rsync $RSYNC_OPTS --backup-dir=$IDIR $path $BACKUP_DIR/$name/
}


############################
# grab a lock file. Not atomic, but close :)
# tries to cope with NFS
lock_file() {
	lck="$1"
	machine=`cat "$lck" 2> /dev/null | cut -d: -f1`
	pid=`cat "$lck" 2> /dev/null | cut -d: -f2`

	if [ -f "$lck" ] && 
	    ( [ $machine != $host ] || kill -0 $pid ) 2> /dev/null; then
		echo "lock file $lck is valid for $machine:$pid"
		return 1
	fi
	/bin/rm -f "$lck"
	echo "$host:$$" > "$lck"
	return 0
}

############################
# unlock a lock file
unlock_file() {
	lck="$1"
	/bin/rm -f "$lck"
}




(
  if ! lock_file $LOCKFILE; then
    exit 1
  fi

  trap "unlock_file $LOCKFILE" EXIT

  ########################
  # list of backups to perform
  backup_one / root
  backup_one /home/ home

  unlock_file backup.lock

) >> $LOGFILE 2>&1

