#!/bin/sh


# wait for a pid with given timeout
# returns 1 if it timed out, 0 if the process exited itself
waitforpid() {
    pid=$1
    timeout=$2 # in seconds
    _wcount=0
    while kill -0 $pid 2> /dev/null; do
	sleep 1;
	_wcount=`expr $_wcount + 1`
	if [ $_wcount -eq $timeout ]; then
	    return "1";
	fi
    done
    return "0";
}

if `waitforpid $1 $2`; then
    echo "process $1 exited"
else
    echo "process $1 timed out"
fi
