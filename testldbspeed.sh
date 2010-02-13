#!/bin/bash

[ $# -eq 5 ] || {
cat <<EOF
usage: testldbspeed.sh DEVICE FSTYPE MNTOPTIONS MNTPOINT LDBTEST

example usage:

  testldbspeed.sh /dev/sda6 ext3 "-o barrier=1" /mnt/ldbtest "/home/tridge/samba/git/combined/source4/bin/ldbtest -H test.ldb --num-records=200 --num-searches=10"

  testldbspeed.sh /dev/sda6 ext4 "-o barrier=0" /mnt/ldbtest "/home/tridge/samba/git/combined/source4/bin/tdbtorture -t -n 1 -s 1"

Test results:
   using tdbtorture -t -n 1 -l 5000 -s 1
   2.6.31-19-generic on x86_64 Ubuntu Karmic. 
   2G partition on a 500G Seagate Momentus 7200.4 SATA disk
   T61 Thinkpad

   ext2  (defaults)          1.7s

   ext3  (defaults)          2.0s
   ext3 -o barrier=1         11.5s

   ext4  (defaults)          35.6s
   ext4 -o nobarrier         2.8s

   btrfs (defaults)          104s
   btrfs -o nobarrier        27.5s

   xfs   (defaults)          14.1s

   jfs   (defaults)          2.0s
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
sync; sync; sleep 1;
time $LDBTEST
sync
popd
umount "$DEV"
