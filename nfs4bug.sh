#!/bin/sh


(
exec >> nfs4bug.dat 2>&1
echo hello
) >> nfs4bug.dat 2>&1

cat nfs4bug.dat
rm -f nfs4bug.dat
exit 0
