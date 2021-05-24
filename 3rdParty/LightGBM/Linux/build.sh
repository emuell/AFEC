#!/bin/bash
CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

DIST_DIR=$SCRIPT_DIR/../Dist
BUILD_DIR=$SCRIPT_DIR/Build

# cd into build dir
mkdir -p $BUILD_DIR 
cd $BUILD_DIR

# configure
cmake $DIST_DIR -DBUILD_STATIC_LIB=on -DUSE_OPENMP=OFF || {
  echo "** configure FAILED"; exit 1
}

# build
export VERBOSE=1
make _lightgbm -j4 || {
  echo "** make FAILED"; exit 1
}

# harvest results
cp $DIST_DIR/lib_lightgbm.a $SCRIPT_DIR/libLightGBM_.a || {
  echo "** cp liblightgbm FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"

