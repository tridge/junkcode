#!/bin/bash

mkdir -p documents/word
mkdir -p documents/other

for f in $*; do
    if `grep MSWordDoc $f > /dev/null`; then
	fname=`basename $f`
	echo documents/word/$fname
	ln $f documents/word/$fname
    else
	fname=`basename $f`
	echo documents/other/$fname
	ln $f documents/other/$fname
    fi
done