set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMFeatureExtraction")

include_directories(../../../3rdParty/Aubio/Dist/src)
include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/Msgpack/Dist/include)
include_directories(../../../3rdParty/LibXtract/Dist/include)
include_directories(../../../3rdParty/OpenBLAS/Dist)
include_directories(../../../3rdParty/Resample/Dist/include)
include_directories(../../../3rdParty/Shark/Dist/include)
include_directories(../../../3rdParty/Sqlite/Dist/src)
if(WITH_INTEL_IPP)
  include_directories(../../../3rdParty/IPP/Dist)
endif()

include_directories(Source)

project_source_files(PROJECT_SOURCE_FILES)
add_library(FeatureExtraction ${PROJECT_SOURCE_FILES})
set_property(TARGET FeatureExtraction PROPERTY FOLDER "Crawler")
