#!/bin/sh

# this script should be . included into the fns file
# of any system that needs an update to its python install

PATH=$HOME/python/bin:$PATH
export PATH

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
