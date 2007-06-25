#!/bin/bash

PSERVER=pserver
#PSERVER=tridge

# need to exclude the dir with rsync
ORIGDIR=`pwd`
PROJECT=`basename $ORIGDIR`
TDIR=../tmpconfig/$PROJECT
SVNFILE=$TDIR.svn
rm -rf $TDIR || exit 1

# see if its worth doing a configure
if [ -f "$SVNFILE" ] && cmp .svn/entries "$SVNFILE" 2> /dev/null; then
    echo "Nothing changed in svn for $PROJECT"
    exit 0
fi
mkdir -p $TDIR

cp .svn/entries $SVNFILE || exit 1

echo running configure for $ORIGDIR

cp -al . $TDIR || exit 1

(
    cd $TDIR || exit 1
    test -d source && cd source
    find . -type d -print0 | xargs -0 chgrp $PSERVER
    find . -type d -print0 | xargs -0 chmod g+w
    rm -f configure config.h.in include/config.h.in
    rm -rf autom4te*
    su -c ./autogen.sh $PSERVER
    rm -rf autom4te*
) || exit 1

cd .. || exit 1
rsync -a --delete tmpconfig/$PROJECT/ $PROJECT/
rm -rf tmpconfig/$PROJECT
