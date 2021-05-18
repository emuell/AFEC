set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMCoreFileFormats")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  pkg_check_modules(MPG123 libmpg123 REQUIRED)
  if(NOT MPG123_FOUND)
    message(FATAL_ERROR "libmpg123 is required for linux builds")
  else()
    include_directories(${MPG123_INCLUDE_DIRS})
  endif()
endif()

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/OggVorbis/Dist/include)
include_directories(../../../3rdParty/Sqlite/Dist/src)
include_directories(../../../3rdParty/Flac/Dist/include)

include_directories(Source)

project_source_files(PROJECT_SOURCE_FILES)
add_library(CoreFileFormats ${PROJECT_SOURCE_FILES})
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	add_precompiled_header(
		CoreFileFormats 
		CoreFileFormatsPrecompiledHeader.h 
		SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/Source/CoreFileFormatsPrecompiledHeader.cpp)
endif()

set_property(TARGET CoreFileFormats PROPERTY FOLDER "Core")
