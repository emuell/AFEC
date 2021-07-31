#!/usr/bin/env bash

# Builds the crawler and model creator and tester tools:
# Pass "-d or --debug" to build debug configs, -r or --release for release configs
# Pass "-c or --clean" to clean before building

SCRIPT_DIR=`dirname -- "$0"`

#### process options

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

### create out dir

OUT_DIR=""
PROJECTS_DIR=""
if [ "$(uname)" == "Darwin" ]; then
  OUT_DIR="$SCRIPT_DIR/Out"
  SOURCE_DIR="../../Source"
else
  OUT_DIR="$SCRIPT_DIR/Out/$BUILD_CONFIG"
  SOURCE_DIR="../../../Source"
fi

if [[ ! -e "$OUT_DIR" ]]; then
  mkdir -p "$OUT_DIR" || {
    echo "*** build failed"; exit 1
  }
fi

### configure and build

CMAKE_TARGET_OPTIONS="-DBUILD_CRAWLER=ON -DBUILD_TESTS=ON"

cd $OUT_DIR || {
  echo "*** can't cd into out dir"; exit 1
}

if [ "$(uname)" == "Darwin" ]; then
  # force compiling x86_64 on Apple M1 systems
  CMAKE_TARGET_OPTIONS="$CMAKE_TARGET_OPTIONS -DCMAKE_APPLE_SILICON_PROCESSOR=x86_64"
  # generate xcode projects on darwin
  cmake -G Xcode $CMAKE_TARGET_OPTIONS $SOURCE_DIR || { 
    echo "*** cmake configure failed"; exit 1
  }
  # build
  cmake --build . --config $BUILD_CONFIG $CLEAN_FLAGS || {
    echo "*** build failed"; exit 1
  }
else
  # use ninja on all other platforms
  cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_CONFIG $CMAKE_TARGET_OPTIONS $SOURCE_DIR || {
    echo "*** cmake configure failed"; exit 1
  }
  # build
  CPU_COUNT=$(grep ^cpu\\scores /proc/cpuinfo | uniq | awk '{print $4}')
  cmake --build . --config $BUILD_CONFIG $CLEAN_FLAGS -- -j$CPU_COUNT || {
    echo "*** build failed"; exit 1
  }
fi

