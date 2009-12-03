#!/bin/sh
# a incremental rsync backup script

LOCKFILE=/var/lock/backup.lock
LOGFILE=/var/log/backup.log

# load config options
. /opt/rsync_backup/config

PASSWORD="--password-file=/opt/rsync_backup/password"

RSYNC_OPTS="$PASSWORD --numeric-ids --delete-after --bwlimit=$BANDWIDTH_LIMIT --exclude-from=$EXCLUDE_FILE --ignore-errors --delete --delete-excluded -axvSH --backup --timeout=$TIMEOUT"

##################################
# send an email on success 
success() {
    echo "`date` Backup of $name on $SERVER succeeded"
    date | mailx -s "rsync backup of $name initiated on $SERVER" $NOTIFY_LIST
}


##################################
# send an email on failure
failed() {
    echo "`date` Backup of $name on $SERVER failed"
    date | mailx -s "backup of $name failed on $SERVER" $NOTIFY_LIST
    exit 1
}


##################################
# backup one filesystem
backup_one() {
    path="$1"
    if [ "$path" = "/" ]; then
	name="root"
    else
	name="`basename $path`"
    fi

    DAY=`date +%j`
    DAY="`expr $DAY % $NUM_INCREMENTAL`"
    IDIR="incremental/$name/$DAY"

    echo "`date` Backing up $name to $path and $IDIR"

    rm -rf /var/rsync_backup/empty
    mkdir -p /var/rsync_backup/empty

    # create the incremental tree if it doesn't exist
    rsync $PASSWORD -r /var/rsync_backup/empty/ $SERVER@$BACKUP_SERVER::$SERVER/incremental ||
    failed "create incremental directory"
    
    # create the incremental subdir if needed
    rsync $PASSWORD -r /var/rsync_backup/empty/ $SERVER@$BACKUP_SERVER::"$SERVER/incremental/$name" || 
    failed "create incremental $SERVER/incremental/$name"

    # delete the old incremental tree for this incremental, if any
    rsync $PASSWORD -a --delete /var/rsync_backup/empty/ $SERVER@$BACKUP_SERVER::"$SERVER/$IDIR" || 
    failed "delete incremental $SERVER/$IDIR"

    touch $path/.backup_stamp

    rsync $PASSWORD $RSYNC_OPTS --backup-dir="/$IDIR" $path/ $SERVER@$BACKUP_SERVER::"$SERVER/$name"
    rc=$?
    case $rc in
	0)  
        24) # partial transfer is OK
	   success "rsync backup $SERVER/$name" 
	   ;;
        *)
	   failed "rsync backup $SERVER/$name with error code $rc" 
	   ;;
   esac
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

  for f in $FILESYSTEMS; do
      backup_one $f
  done

  unlock_file $LOCKFILE

) >> $LOGFILE 2>&1

