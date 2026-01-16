CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

SRCDIR=$SCRIPT_DIR/../Dist
cd $SRCDIR

export CFLAGS="-fPIC -fvisibility=hidden"
export CXXFLAGS="-fPIC -fvisibility=hidden -fvisibility-inlines-hidden"

# arm32 build: --build=aarch64-linux-gnu --host=arm-linux-gnueabihf 

chmod +x ./configure && ./configure --disable-ogg --enable-shared=NO --enable-static=YES
if [[ $? != 0 ]] ; then
  echo "** configure FAILED"
  exit $?
fi

make -C src/libFLAC && make -C src/libFLAC++
if [[ $? != 0 ]] ; then
  echo "** make libFLAC FAILED"
  exit $?
fi

cp src/libFLAC/.libs/libFLAC-static.a $SCRIPT_DIR/libFlac_.a  &&
  cp src/libFLAC++/.libs/libFLAC++-static.a $SCRIPT_DIR/libFlac++_.a  || \
if [[ $? != 0 ]] ; then
  echo "** cp libFLAC FAILED"
  exit $?
fi

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
