#!/bin/sh

# this script downloads and installs python in $HOME/python

VERSION="Python-2.6.5"

set -e

cd $HOME
mkdir -p python
cd python
rsync -Pavz samba.org::ftp/tridge/python/$VERSION.tar . || exit 1
tar -xf $VERSION.tar || exit 1
cd $VERSION || exit 1
./configure --prefix=$HOME/python --disable-ipv6 || exit 1
make || exit 1
make install || exit 1

$HOME/python/bin/python -V

echo 'Please put $HOME/python/bin in your $PATH'
