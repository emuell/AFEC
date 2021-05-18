#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

# set to 1 to enable intel IPP in aubio
WITH_INTEL_IPP=0

if [ $WITH_INTEL_IPP -eq 1 ]; then
  IPP_PATH="$SCRIPT_DIR/../../IPP"

  if [ `uname -m` == "x86_64" ]; then
    LIB_ARCH="x86_64"
  elif [ `uname -m` == "i686" ]; then
    LIB_ARCH="x86"
  else
    echo "** unexpected architecture: `uname -m`"; exit 1
  fi

  export CFLAGS="-I$IPP_PATH/Dist -fPIC -fvisibility=hidden"
  export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"
  export LDFLAGS="-L$IPP_PATH/Library/Linux/$LIB_ARCH"

  $SCRIPT_DIR/waf configure --enable-double --enable-intelipp || {
    echo "** configure FAILED"; exit 1
  }
else
  export CFLAGS="-fvisibility=hidden"
  export CXXFLAGS="$CFLAGS -fvisibility-inlines-hidden"

  $SCRIPT_DIR/waf configure --enable-double --disable-intelipp || {
    echo "** configure FAILED"; exit 1
  }
fi

# NB: build will fail, but will produce a static lib
$SCRIPT_DIR/waf build || {
  echo "** make FAILED - nevertheless checking if a static lib was produced..."; # exit 1
}

cp build/src/libaubio.a $SCRIPT_DIR/libAubio_.a || {
  echo "** cp FAILED"; exit 1
}

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
