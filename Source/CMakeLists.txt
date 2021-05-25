###################################################################################################

# Global cmake config
cmake_minimum_required(VERSION 2.8)

# Set CMAKE module path
set(CMAKE_MODULE_PATH
    "${CMAKE_MODULE_PATH}"
    "${CMAKE_SOURCE_DIR}/../Build/CMakeModules")

# Set policies
if(POLICY CMP0063)
  # use new CMAKE_CXX_VISIBILITY_PRESET behavior
  cmake_policy(SET CMP0063 NEW)
endif()

if(WIN32)
  # ensure Windows 8.1 Desktop SDKs is used in Visual CPP solutions
  set(CMAKE_SYSTEM_VERSION 8.1 CACHE STRING INTERNAL FORCE)
  # enable solution folders for Visual CPP
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif(WIN32)

if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  # ensure CMAKE_RUNTIME_OUTPUT_DIRECTORY points to an abs path:
  # cmake-js sets it to a relatove path, which is causing weired problems
  get_filename_component(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
endif()

# Set project name
project(AFEC)

###################################################################################################

# Options
option(BUILD_CRAWLER "Build the Crawler and Classification Model Tools" OFF)
option(BUILD_TESTS "Build Unit Tests" OFF)
option(BUILD_EXAMPLES "Build Example Apps" OFF)

option(WITH_INTEL_IPP "Build with Intel IPP support" OFF)

# Enable all targets except node modules by default
if (NOT BUILD_TESTS AND
    NOT BUILD_CRAWLER)
  message(STATUS "Note: Building crawler and tests as no targets have been specified.")
  set(BUILD_CRAWLER ON CACHE BOOL "Build the Crawler and Classification Model Tools" FORCE)
  set(BUILD_TESTS ON CACHE BOOL "Build Unit Tests" FORCE)
endif()

# Include shared project helper modules
include(ProjectCopyExecutable)
include(ProjectSourceFiles)
include(PrecompiledHeader)

# Check build type and restrict to "Debug" and "Release" only
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
      "Available build configurations"
      FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Note: Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(FATAL_ERROR "Invalid/unknown CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'.")
endif()

###################################################################################################

# Package dependencies (for Linux builds only)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(PkgConfig REQUIRED)
  find_package(Threads REQUIRED)
endif()

###################################################################################################

# Architecture specific flags
include(TargetArchitecture)
target_architecture(ARCH)

# Build configuration specific flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMDebug")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DMRelease")

if(ARCH MATCHES "x86_64")
  # 64bit builds
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMArch_X64")
elseif(ARCH MATCHES "i386")
  # 32bit builds
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMArch_X86")
else()
  message(FATAL_ERROR "Unexpected or unsupported ARCH: ${ARCH}")
endif()

# Compiler specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # G++/Clang flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -DMCompiler_GCC")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMCompiler_Clang") # yes, GCC AND Clang
  endif()
  if(ARCH MATCHES "i386")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
  endif()
  # visibility
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  # build everything with fPIC: we're using the core libraries also in shared libraries
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  # warnings
  set(WARNING_FLAGS "-Wall -Wno-unknown-warning-option"
      "-Wno-reorder -Wno-unused-private-field -Wno-unused-local-typedefs"
      "-Wno-null-dereference -Wno-deprecated-declarations -Wno-unknown-pragmas"
      "-Wno-expansion-to-defined")
  string(REGEX REPLACE ";" " " WARNING_FLAGS "${WARNING_FLAGS}") # list -> string
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMCompiler_VisualCPP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DDEBUG /Zi")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE -D_ITERATOR_DEBUG_LEVEL=0 -D_SECURE_SCL=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /GS- /GF /Gy /W4 /WX /bigobj")
  if(ARCH MATCHES "i386")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
  endif()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Ox /Ot /Ob1")
  # MSVC linker flags
  set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /IGNORE:4221 /DEBUG")
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libcmt.lib")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF")
else()
  message(FATAL_ERROR "Unexpected or unsupported CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Platform specific flags
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Linux Systems
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMLinux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # Osx Systems
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMMac")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMNoMp3LibMpg")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Windows Systems
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -D_WIN32_WINDOWS=0x0601")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMWindows")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMNoMp3LibMpg")
else()
  message(FATAL_ERROR "Unexpected or unsupported CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
endif()

# Configuration specific flags
if(WITH_INTEL_IPP)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMHaveIntelIPP")
endif()

###################################################################################################

# set LINKER_PLATFORM
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINKER_PLATFORM "Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(LINKER_PLATFORM "Mac")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(LINKER_PLATFORM "Win")
endif()

# set LINKER_ARCH
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(ARCH MATCHES "x86_64")
    set(LINKER_ARCH "x86_64")
  elseif(ARCH MATCHES "i386")
    set(LINKER_ARCH "x86")
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(LINKER_ARCH "") # universal binaries on OSX
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  if(ARCH MATCHES "x86_64")
    set(LINKER_ARCH "x64")
  elseif(ARCH MATCHES "i386")
    set(LINKER_ARCH "x86")
  endif()
endif()

# make all "3rdParty" lib dirs visible to the linker
set(THIRD_PARTY_LIBRARIES
    "Aubio" "Boost" "Flac" "Iconv" "IPP" "LibXtract" "LightGBM" "Mpg123" 
    "Ogg" "OggVorbis" "OpenBLAS" "Resample" "Shark" "Sqlite" "ZLib")

# add link directories for all third party libs
foreach(THIRD_PARTY_LIB ${THIRD_PARTY_LIBRARIES})
  # add link directories for all third party libs
  get_filename_component(LIBRARY_DIR
      "../3rdParty/${THIRD_PARTY_LIB}/Library/${LINKER_PLATFORM}/${LINKER_ARCH}"
      ABSOLUTE)
  if(EXISTS ${LIBRARY_DIR})
    link_directories(${LIBRARY_DIR})
  endif()
endforeach()

###################################################################################################

# include sub projects
add_subdirectory(Core)
add_subdirectory(Crawler)
