set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMUnitTests")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/Msgpack/Dist/include)
include_directories(../../../3rdParty/OpenBLAS/Dist)
include_directories(../../../3rdParty/Shark/Dist/include)
include_directories(../../../3rdParty/Sharkonvnet/Dist/src)

project_source_files(PROJECT_SOURCE_FILES)
add_executable(XUnitTests ${PROJECT_SOURCE_FILES})
set_property(TARGET XUnitTests PROPERTY FOLDER "Crawler")

# internal lib dependencies
target_link_libraries(XUnitTests FeatureExtraction)
target_link_libraries(XUnitTests Classification)

target_link_libraries(XUnitTests CoreFileFormats)
target_link_libraries(XUnitTests AudioTypes)
target_link_libraries(XUnitTests CoreTypes)

# third party lib dependencies
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (WITH_INTEL_IPP)
    set(IPP_LIBS "ippi;ipps;ippvm;ippcore;imf;irc;svml")
  else()
    set(IPP_LIBS "")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(XUnitTests
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostUnitTest_;BoostSystem_;BoostSerialization_;"
      "VorbisFile_;Vorbis_;VorbisEncode_;Ogg_;Flac++_;Flac_;"
      "Sqlite_;Iconv_;Z_;${IPP_LIBS};pthread;dl;rt")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(XUnitTests
      "Aubio_;Xtract_;Resample_;Shark_;LightGBM_;"
      "BoostUnitTest_;BoostSystem_;BoostSerialization_;"
      "OggVorbis_;Flac_;Sqlite_;Iconv_;z;${IPP_LIBS}")
    target_link_libraries(XUnitTests "-framework Carbon")
    target_link_libraries(XUnitTests "-framework CoreFoundation")
    target_link_libraries(XUnitTests "-framework CoreServices")
    target_link_libraries(XUnitTests "-framework Accelerate")
    target_link_libraries(XUnitTests "-framework AppKit")
    target_link_libraries(XUnitTests "-framework AudioToolBox")
    target_link_libraries(XUnitTests "-framework IOKit")
  else()
    message(FATAL_ERROR "Unexpected platform/compiler setup")
  endif()
else()
  # mscv builds add libraries via #pragma linker preprocess commands
endif()

# executable dependencies
add_dependencies(XUnitTests XCrawler)
add_dependencies(XUnitTests XModelCreator)

# copy into Binary directory
project_copy_executable(XUnitTests EXECUTABLE_DEST_PATH)

# auto-run test after build (in release configurations only - tests are slow)
if (CMAKE_GENERATOR MATCHES "^Visual Studio" OR
	  CMAKE_GENERATOR MATCHES "^Xcode" OR
    CMAKE_BUILD_TYPE STREQUAL "Release")
  set(unittest_stamp
    "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XUnitTests.stamp")
  set(UNIT_TEST_ARGUMENTS "--log_level=message")
  if (CMAKE_GENERATOR MATCHES "^Visual Studio")
    add_custom_command(OUTPUT "${unittest_stamp}"
      COMMAND if 1 == $<CONFIG:Release> (
          ${EXECUTABLE_DEST_PATH} ${UNIT_TEST_ARGUMENTS}
        )
      COMMAND ${CMAKE_COMMAND} -E touch "${unittest_stamp}"
      POST_BUILD
      DEPENDS XUnitTests)
  elseif (CMAKE_GENERATOR MATCHES "^Xcode")
    add_custom_command(OUTPUT "${unittest_stamp}"
      COMMAND [ $<CONFIG:Release> -eq 1 ] && ${EXECUTABLE_DEST_PATH} ${UNIT_TEST_ARGUMENTS}
      COMMAND ${CMAKE_COMMAND} -E touch "${unittest_stamp}"
      POST_BUILD
      DEPENDS XUnitTests)
  else()
    add_custom_command(OUTPUT "${unittest_stamp}"
      COMMAND ${EXECUTABLE_DEST_PATH} ${UNIT_TEST_ARGUMENTS}
      COMMAND ${CMAKE_COMMAND} -E touch "${unittest_stamp}"
      POST_BUILD
      DEPENDS XUnitTests)
  endif()
  add_custom_target(RUN_CRAWLER_TESTS ALL DEPENDS "${unittest_stamp}")
  set_property(TARGET RUN_CRAWLER_TESTS PROPERTY FOLDER "CMakePredefinedTargets")
endif()
