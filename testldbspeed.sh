#!/bin/bash

[ $# -eq 5 ] || {
cat <<EOF
usage: testldbspeed.sh DEVICE FSTYPE MNTOPTIONS MNTPOINT LDBTEST

example usage:

  testldbspeed.sh /dev/sda6 ext3 "-o barrier=1" /mnt/ldbtest "/home/tridge/samba/git/combined/source4/bin/ldbtest -H test.ldb --num-records=200 --num-searches=10"
EOF
    exit 1
}

DEV="$1"
FSTYPE="$2"
MNTOPTIONS="$3"
MNTPOINT="$4"
LDBTEST="$5"

grep "$DEV" /proc/mounts && {
    umount -f "$DEV" || exit 1
}
mkfs."$FSTYPE" "$DEV" || exit 1
mkdir -p "$MNTPOINT" || exit 1
mount "$DEV" $MNTOPTIONS "$MNTPOINT" || exit 1

pushd "$MNTPOINT" || exit 1
time $LDBTEST
sync
popd
umount "$DEV"
