#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH
SRCDIR=$SCRIPT_DIR/../Dist/src

export CFLAGS="-fPIC -fvisibility=hidden"

gcc -c $SRCDIR/sqlite3.c -o $SRCDIR/sqlite3.o $CFLAGS || {
  echo "** gcc FAILED"; exit 1
}

ar rcs $SCRIPT_DIR/libSqlite_.a $SRCDIR/sqlite3.o || {
  echo "** ar FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
