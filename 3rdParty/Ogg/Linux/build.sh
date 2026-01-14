CURDIR=`pwd`; cd `dirname $0`; ABSPATH=`pwd`; cd $CURDIR
SCRIPT_DIR=$ABSPATH

cd $SCRIPT_DIR/../Dist

export CFLAGS="-fPIC -fvisibility=hidden"
export CXXFLAGS='$CFLAGS -fvisibility-inlines-hidden'

# arm32 build: --build=aarch64-linux-gnu --host=arm-linux-gnueabihf 

sh autogen.sh --enable-static=YES --enable-shared=NO || \
  (echo "** autogen FAILED" && exit 1)

make || \
  (echo "** make FAILED" && exit 1)

cp src/.libs/libogg.a $SCRIPT_DIR/libOgg_.a || \
  (echo "** cp FAILED" && exit 1)

echo "** Build succeeded -> `ls $SCRIPT_DIR/*.a`"
