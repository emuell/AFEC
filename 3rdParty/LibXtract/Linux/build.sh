#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

export CFLAGS="-fPIC -fvisibility=hidden"
export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"

make -C src || {
  echo "** make FAILED"; exit 1
}

cp src/libxtract.a $SCRIPT_DIR/libXtract_.a || {
  echo "** cp FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"

