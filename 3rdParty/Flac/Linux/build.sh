#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

export CFLAGS="-fPIC -fvisibility=hidden"
export CXXFLAGS="-fPIC -fvisibility=hidden -fvisibility-inlines-hidden"

chmod +x ./configure && ./configure --disable-doxygen-docs --enable-shared=NO --enable-static=YES || {
  echo "** configure FAILED"; exit 1
}

make -C src/libFLAC && make -C src/libFLAC++ || {
  echo "** make libFLAC FAILED"; exit 1
}

cp src/libFLAC/.libs/libFLAC-static.a $SCRIPT_DIR/libFlac_.a  &&
  cp src/libFLAC++/.libs/libFLAC++-static.a $SCRIPT_DIR/libFlac++_.a || {
  echo "** cp libFLAC FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"

