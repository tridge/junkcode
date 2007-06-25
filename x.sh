#!/bin/sh

if [ -e foo.dat -a "`cat foo.dat 2>/dev/null`" = foo ]; then
    echo ok
fi
