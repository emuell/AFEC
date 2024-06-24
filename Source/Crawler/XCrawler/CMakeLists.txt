set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMCrawler")

include_directories(../../../3rdParty/Boost/Dist)

project_source_files(PROJECT_SOURCE_FILES)
add_executable(XCrawler ${PROJECT_SOURCE_FILES})
set_property(TARGET XCrawler PROPERTY FOLDER "Crawler")

# internal lib dependencies
target_link_libraries(XCrawler FeatureExtraction)
target_link_libraries(XCrawler Classification)

target_link_libraries(XCrawler CoreFileFormats)
target_link_libraries(XCrawler AudioTypes)
target_link_libraries(XCrawler CoreTypes)

# third party lib dependencies
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (WITH_INTEL_IPP)
    set(IPP_LIBS "ippi;ipps;ippvm;ippcore;imf;irc;svml")
  else()
    set(IPP_LIBS "")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(XCrawler
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostSystem_;BoostSerialization_;BoostProgramOptions_;"
      "VorbisFile_;Vorbis_;VorbisEncode_;Ogg_;Flac++_;Flac_;"
      "Sqlite_;Iconv_;Z_;${IPP_LIBS};pthread;dl;rt")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(XCrawler
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostSystem_;BoostSerialization_;BoostProgramOptions_;"
      "OggVorbis_;Flac_;Sqlite_;Iconv_;z;${IPP_LIBS}")
    target_link_libraries(XCrawler "-framework CoreFoundation")
    target_link_libraries(XCrawler "-framework CoreServices")
    target_link_libraries(XCrawler "-framework AppKit")
    target_link_libraries(XCrawler "-framework AudioToolBox")
    target_link_libraries(XCrawler "-framework IOKit")
    target_link_libraries(XCrawler "-framework Accelerate")
  else()
    message(FATAL_ERROR "Unexpected platform/compiler setup")
  endif()
else()
  # mscv builds add libraries via #pragma linker preprocess commands
endif()

# copy executable to "Dist" directory
project_copy_executable(XCrawler)

# copy default models to "Dist" directory
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Models)
if (CMAKE_GENERATOR MATCHES "^Visual Studio" OR
    CMAKE_GENERATOR MATCHES "^Xcode")
  set(TARGET_PATH ${CMAKE_SOURCE_DIR}/../Dist/${CMAKE_CFG_INTDIR}/Models)
else()
  set(TARGET_PATH ${CMAKE_SOURCE_DIR}/../Dist/${CMAKE_BUILD_TYPE}/Models)
endif()
add_custom_command(TARGET XCrawler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGET_PATH}
    COMMAND ${CMAKE_COMMAND} -E echo \"copy ${SRC_PATH} -> ${TARGET_PATH}\"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SRC_PATH} ${TARGET_PATH})
