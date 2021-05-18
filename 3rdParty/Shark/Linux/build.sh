#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

export CFLAGS="-fPIC -fvisibility=hidden"
export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"

cmake -DBoost_INCLUDE_DIR="$SRCDIR/../../Boost/Dist" || {
  echo "** cmake configure FAILED"; exit 1
}

cmake --build . --target shark || {
  echo "** cmake build FAILED"; exit 1
}

cp $SRCDIR/lib/libshark.a $SCRIPT_DIR/libShark_.a || {
  echo "** cp FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
