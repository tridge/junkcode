#!/bin/sh
# basic http based install script for clipper
# most of the actual work is done by the scripts inside root.tgz,
# this script just fetches it and launches that script
# tridge@samba.org, December 2001

echo Installing from $archive_location
echo Using install script $install_script

if [ ! -z "$proxy_server" ]; then
    echo Using Proxy $proxy_server
    export http_proxy="$proxy_server"
    export ftp_proxy="$proxy_server"
fi

echo Building install ramdisk
umount /install
mke2fs -r 0 -s 1 -q -b 1024 /dev/ram1 4000 || exit 1
mkdir -p /install
mount /dev/ram1 /install || exit 1

echo Fetching archive
wget --passive-ftp -O /install/root.tgz $archive_location || exit 1

cd /install || exit 1

tar xvzf root.tgz $install_script || exit 1

echo Starting $install_script

exec $install_script root.tgz
