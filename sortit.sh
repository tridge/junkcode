#!/bin/bash

mkdir -p photos/other

for f in $*; do
    d=`jhead $f| grep ^Date/Time | cut -c16-26| sed 's|:|/|g'`
    if [ -z "$d" ]; then
	fname=`basename $f`
	echo photos/other/$fname
	ln $f photos/other/$fname
	continue;
    fi

    dir=`echo $d | cut -d/ -f1-2`
    fname=`basename $f`
    mkdir -p photos/$dir
    echo photos/$dir/$fname
    ln $f photos/$dir/$fname
done