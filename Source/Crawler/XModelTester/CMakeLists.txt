set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMModelTester")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/OpenBLAS/Dist)
include_directories(../../../3rdParty/Shark/Dist/include)
include_directories(../../../3rdParty/Sharkonvnet/Dist/src)

project_source_files(PROJECT_SOURCE_FILES)
add_executable(XModelTester ${PROJECT_SOURCE_FILES})
set_property(TARGET XModelTester PROPERTY FOLDER "Crawler")

# internal lib dependencies
target_link_libraries(XModelTester FeatureExtraction)
target_link_libraries(XModelTester Classification)

target_link_libraries(XModelTester CoreFileFormats)
target_link_libraries(XModelTester AudioTypes)
target_link_libraries(XModelTester CoreTypes)

# third party lib dependencies
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(WITH_INTEL_IPP)
    set(IPP_LIBS "ippi;ipps;ippvm;ippcore;imf;irc;svml")
  else()
    set(IPP_LIBS "")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(XModelTester
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostSystem_;BoostSerialization_;BoostProgramOptions_;"
      "VorbisFile_;Vorbis_;VorbisEncode_;Ogg_;Flac++_;Flac_;"
      "Sqlite_;Iconv_;Z_;${IPP_LIBS};pthread;dl;rt")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(XModelTester
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostSystem_;BoostSerialization_;BoostProgramOptions_;"
      "OggVorbis_;Flac_;Sqlite_;Iconv_;z;${IPP_LIBS}")
    target_link_libraries(XModelTester "-framework CoreFoundation")
    target_link_libraries(XModelTester "-framework CoreServices")
    target_link_libraries(XModelTester "-framework AppKit")
    target_link_libraries(XModelTester "-framework AudioToolBox")
    target_link_libraries(XModelTester "-framework IOKit")
    target_link_libraries(XModelTester "-framework Accelerate")
  else()
    message(FATAL_ERROR "Unexpected platform/compiler setup")
  endif()
else()
  # mscv builds add libraries via #pragma linker preprocess commands
endif()

# copy executable to "Dist" directory
project_copy_executable(XModelTester)
