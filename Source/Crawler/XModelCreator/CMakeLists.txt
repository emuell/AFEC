set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMModelCreator")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/OpenBLAS/Dist)
include_directories(../../../3rdParty/Shark/Dist/include)
include_directories(../../../3rdParty/Sharkonvnet/Dist/src)

project_source_files(PROJECT_SOURCE_FILES)
add_executable(XModelCreator ${PROJECT_SOURCE_FILES})
set_property(TARGET XModelCreator PROPERTY FOLDER "Crawler")

# internal lib dependencies
target_link_libraries(XModelCreator FeatureExtraction)
target_link_libraries(XModelCreator Classification)

target_link_libraries(XModelCreator CoreFileFormats)
target_link_libraries(XModelCreator AudioTypes)
target_link_libraries(XModelCreator CoreTypes)

# third party lib dependencies
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(WITH_INTEL_IPP)
    set(IPP_LIBS "ippi;ipps;ippvm;ippcore;imf;irc;svml")
  else()
    set(IPP_LIBS "")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(XModelCreator
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostSystem_;BoostSerialization_;BoostProgramOptions_;"
      "VorbisFile_;Vorbis_;VorbisEncode_;Ogg_;Flac++_;Flac_;"
      "Sqlite_;Iconv_;Z_;${IPP_LIBS};pthread;dl;rt")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(XModelCreator
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostSystem_;BoostSerialization_;BoostProgramOptions_;"
      "OggVorbis_;Flac_;Sqlite_;Iconv_;z;${IPP_LIBS}")
    target_link_libraries(XModelCreator "-framework CoreFoundation")
    target_link_libraries(XModelCreator "-framework CoreServices")
    target_link_libraries(XModelCreator "-framework AppKit")
    target_link_libraries(XModelCreator "-framework AudioToolBox")
    target_link_libraries(XModelCreator "-framework IOKit")
    target_link_libraries(XModelCreator "-framework Accelerate")
  else()
    message(FATAL_ERROR "Unexpected platform/compiler setup")
  endif()
else()
  # mscv builds add libraries via #pragma linker preprocess commands
endif()

# copy executable to "Dist" directory
project_copy_executable(XModelCreator)

