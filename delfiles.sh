#!/bin/sh
# measure how much time it takes to delete files/directories in parallel
# tridge@samba.org, July 2007

[ $# -eq 3 ] || {
    echo "usage: delfiles.sh NDIRS NPROCS DIR"
    exit 1
}

NDIRS=$1
NPROCS=$2
DIR=$3

mkdir -p $DIR

echo "Creating files in $NDIRS subdirectories"

for i in `seq 1 $NDIRS`; do
    echo -n "$i "
    mkdir -p $DIR/clients/client$i/{a,b,c,d}/{a,b,c,d}
    touch    $DIR/clients/client$i/{a,b,c,d}/{a,b,c,d}/f{1,2,3,4,5,6,7,8,9,0}
done
echo

echo "Deleting files in parallel using $NPROCS processes"
echo "`date` starting deletion"
t1=`date +%s`
i=1
perproc=`expr $NDIRS / $NPROCS`
for p in `seq 1 $NPROCS`; do
    cmd=""
    end=`expr $i + $perproc - 1`
    for i in `seq $i $end`; do
	cmd="$cmd $DIR/clients/client$i"
    done
    rm -rf $cmd &
    i=`expr $i + 1`
done

wait
echo "`date` done"
t2=`date +%s`
echo "deletion took `expr $t2 - $t1` seconds"
rmdir $DIR/clients
