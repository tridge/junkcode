#!/bin/sh


waitforpid() {
    pid=$1
    timeout=$2 # in seconds
    _wcount=0
    while kill -0 $pid; do
	sleep 1;
	wcount=`expr $wcount + 1`
	if [ $wcount -eq $timeout ]; then
	    last;
	fi
    done
}

waitforpid $1 $2
