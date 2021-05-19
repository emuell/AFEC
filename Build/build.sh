#!/usr/bin/env bash

# Builds the crawler and model creator and tester tools:
# Pass "-d or --debug" to build debug configs, -r or --release for release configs
# Pass "-c or --clean" to clean before building

SCRIPT_DIR=`dirname -- "$0"`

OUT_DIR=$SCRIPT_DIR/Out
if [[ ! -e "$OUT_DIR" ]]; then
  mkdir "$OUT_DIR" || {
    echo "*** build failed"; exit 1
  }
fi

cd "$OUT_DIR"

# process options
BUILD_CONFIG="Release"
CLEAN_FLAGS=""

for i in "$@"
do
  case $i in
    -d|--debug)
      BUILD_CONFIG="Debug"
      ;;
      -r|--release)
      BUILD_CONFIG="Release"
      ;;
      -c|--clean)
      CLEAN_FLAGS="--clean-first"
      ;;
      *)
      # unknown option
      ;;
  esac
done

# configure
CMAKE_TARGET_OPTIONS="-DBUILD_CRAWLER=ON -DBUILD_TESTS=ON"
SOURCE_DIR="../../Source"

if [ "$(uname)" == "Darwin" ]; then
  # generate xcode projects on darwin
  cmake $SOURCE_DIR -G Xcode $CMAKE_TARGET_OPTIONS || { 
    echo "*** cmake configure failed"; exit 1
  }
  # build
  cmake --build . --config $BUILD_CONFIG $CLEAN_FLAGS || {
    echo "*** build failed"; exit 1
  }
else
   # else makefiles with the specified build config
  cmake $SOURCE_DIR -DCMAKE_BUILD_TYPE=$BUILD_CONFIG $CMAKE_TARGET_OPTIONS || {
    echo "*** cmake configure failed"; exit 1
  }
  # build
  CPU_COUNT=$(grep ^cpu\\scores /proc/cpuinfo | uniq | awk '{print $4}')
  cmake --build . --config $BUILD_CONFIG $CLEAN_FLAGS -- -j$CPU_COUNT || {
    echo "*** build failed"; exit 1
  }
fi

