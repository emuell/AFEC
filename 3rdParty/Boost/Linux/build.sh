#!/usr/bin/env bash

CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

CFLAGS="-fPIC -fvisibility=hidden"
CXXFLAGS="--std=c++14 $CFLAGS -fvisibility-inlines-hidden"
# to cross compile 32bit libraries on a 64bit machine, set to 32
ADDRESS_MODEL=64

./bootstrap.sh --with-libraries=system,serialization,test,program_options || {
  echo '** bootstrap configure FAILED'; exit 1
}

./b2 toolset=gcc address-model=$ADDRESS_MODEL link=static threading=single define=BOOST_TEST_NO_MAIN cxxflags="$CXXFLAGS" cflags="$CFLAGS" -j4 || { 
  echo '** b2 make FAILED'; exit 1
}

cp ./stage/lib/libboost_system.a $SCRIPT_DIR/libBoostSystem_.a || {
  echo "** cp FAILED"; exit 1
}
cp ./stage/lib/libboost_serialization.a $SCRIPT_DIR/libBoostSerialization_.a || {
  echo "** cp FAILED"; exit 1
}
cp ./stage/lib/libboost_unit_test_framework.a $SCRIPT_DIR/libBoostUnitTest_.a || {
  echo "** cp FAILED"; exit 1
}
cp ./stage/lib/libboost_program_options.a $SCRIPT_DIR/libBoostProgramOptions_.a || {
  echo "** cp FAILED"; exit 1
}
echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
