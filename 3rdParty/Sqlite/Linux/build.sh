#!/bin/bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

export CFLAGS="-fPIC -fvisibility=hidden"

SRCDIR=$SCRIPT_DIR/../Dist/src

# x86 builds:
# CC='gcc -m32'
# AR='ar'

# arm32 builds:
# CC=arm-linux-gnueabihf-gcc
# AR=arm-linux-gnueabihf-ar

# default
CC=gcc
AR=ar

$CC -c $SRCDIR/sqlite3.c -o $SRCDIR/sqlite3.o $CFLAGS || \
  (echo "** gcc FAILED" && exit 1)

$AR rcs $SCRIPT_DIR/libSqlite_.a $SRCDIR/sqlite3.o || \
  (echo "** ar FAILED" && exit 1)

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
