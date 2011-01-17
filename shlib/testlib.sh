#!/bin/bash

CFLAGS=-Wall

SONAME1=libversion1.so
SONAME2=libversion2.so
SYMVER1=symversion1
SYMVER2=symversion2

clean() {
    rm -rf private public main main2 vscript* *~
}

build() {
    SONAME1="$1"
    SONAME2="$2"
    SYMVER1="$3"
    SYMVER2="$4"
    MSG="$5"
    clean
    mkdir -p private public
    echo "$SYMVER1 { global: *; };" > vscript1
    echo "$SYMVER2 { global: *; };" > vscript2

    gcc $CFLAGS -Wl,-soname=$SONAME1 -Wl,--version-script=vscript1 -DLIBVERSION=\"version1\" -fPIC -shared lib.c -o public/$SONAME1 || exit 1
    gcc -DENABLE_V2 $CFLAGS -Wl,-soname=$SONAME2 -Wl,--version-script=vscript2 -DLIBVERSION=\"version2\" -fPIC -shared lib.c -o private/$SONAME2  || exit 1
    gcc $CFLAGS -Wl,--rpath=$PWD/private -fPIC -shared module.c -o public/module.so private/$SONAME2  || exit 1
    gcc -DENABLE_V2 $CFLAGS -Wl,--rpath=$PWD/private -fPIC -shared module.c -o public/module2.so private/$SONAME2  || exit 1
    gcc $CFLAGS -Wl,--rpath=$PWD/public main.c -o main public/$SONAME1 -ldl  || exit 1
    gcc -DENABLE_V2 $CFLAGS -Wl,--rpath=$PWD/public main.c -o main2 public/$SONAME1 -ldl  || exit 1
    ./main > /dev/null
    status=$?
    case $status in
	0)
	    echo "$MSG: OK: uses same library"
	    ;;
	1)
	    echo "$MSG: ERROR: returns different pointers"
	    ;;
	2)
	    echo "$MSG: ERROR: fails to load module"
	    ;;
	*)
	    echo "$MSG: UNKNOWN return code"
	    ;;
    esac

    ./main2 > /dev/null
    status=$?
    case $status in
	0)
	    echo "$MSG: ERROR: uses same library"
	    ;;
	1)
	    echo "$MSG: ERROR: returns different pointers"
	    ;;
	2)
	    echo "$MSG: OK: fails to load module that depends on new symbol"
	    ;;
	*)
	    echo "$MSG: UNKNOWN return code"
	    ;;
    esac
}

build libversion1.so libversion2.so symversion1 symversion2 "both different"
build libversion1.so libversion1.so symversion1 symversion2 "symbol version different, soname same"
build libversion1.so libversion2.so symversion1 symversion1 "symbol version same, soname different"
build libversion1.so libversion1.so symversion1 symversion1 "both same"

clean
