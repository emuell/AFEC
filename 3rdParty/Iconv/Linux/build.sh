#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

export CFLAGS="-fPIC -fvisibility=hidden"
export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"

chmod +x ./configure || {
  echo "** chmod FAILED"; exit 1
}

./configure --enable-shared=NO --enable-static=YES || {
  echo "** configure FAILED"; exit 1
}

make || {
  echo "** make FAILED"; exit 1
}

cp lib/.libs/libiconv.a $SCRIPT_DIR/libIconv_.a || {
  echo "** cp FAILED" && exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"

