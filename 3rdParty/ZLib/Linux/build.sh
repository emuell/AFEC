#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH
cd $SCRIPT_DIR/../Dist

export CFLAGS="-fPIC -fvisibility=hidden"

chmod +x ./configure || {
  echo "** chmod FAILED"; exit 1
}

./configure --enable-static=YES --enable-shared=NO || {
  echo "** configure FAILED"; exit 1
}

make || {
  echo "** make FAILED"; exit 1
}

cp libz.a $SCRIPT_DIR/libZ_.a || {
  echo "** copy FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"

