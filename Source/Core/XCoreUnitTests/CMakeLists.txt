set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMCoreUnitTests")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/Iconv/Dist/include)
include_directories(../../../3rdParty/ZLib/Dist)
if(WITH_INTEL_IPP)
  include_directories(../../../3rdParty/IPP/Dist)
endif()

project_source_files(PROJECT_SOURCE_FILES)
add_executable(XCoreUnitTests ${PROJECT_SOURCE_FILES})
set_property(TARGET XCoreUnitTests PROPERTY FOLDER "Core")

# internal lib dependencies
target_link_libraries(XCoreUnitTests AudioTypes)
target_link_libraries(XCoreUnitTests CoreFileFormats)
target_link_libraries(XCoreUnitTests CoreTypes)

# third party lib dependencies
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (WITH_INTEL_IPP)
    set(IPP_LIBS "ippi;ipps;ippvm;ippcore;imf;irc;svml")
  else()
    set(IPP_LIBS "")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(XCoreUnitTests
      "Shark_;BoostUnitTest_;BoostSystem_;BoostSerialization_;"
      "VorbisFile_;Vorbis_;VorbisEncode_;Ogg_;Flac++_;Flac_;Sqlite_;"
      "Iconv_;Z_;${IPP_LIBS};pthread;dl;rt")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(XCoreUnitTests
      "Shark_;BoostUnitTest_;BoostSystem_;BoostSerialization_;"
      "OggVorbis_;Flac_;Sqlite_;Iconv_;z;${IPP_LIBS}")
    target_link_libraries(XCoreUnitTests "-framework CoreFoundation")
    target_link_libraries(XCoreUnitTests "-framework CoreServices")
    target_link_libraries(XCoreUnitTests "-framework AppKit")
    target_link_libraries(XCoreUnitTests "-framework AudioToolBox")
    target_link_libraries(XCoreUnitTests "-framework IOKit")
    target_link_libraries(XCoreUnitTests "-framework Accelerate")
  else()
    message(FATAL_ERROR "Unexpected platform/compiler setup")
  endif()
else()
  # mscv builds add libraries via #pragma linker preprocess commands
endif()
  
# copy into Binary directory
project_copy_executable(XCoreUnitTests EXECUTABLE_DEST_PATH)

# auto-run test after build
set(unittest_stamp
  "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/XCoreUnitTests.stamp")
add_custom_command(OUTPUT "${unittest_stamp}"
  COMMAND ${EXECUTABLE_DEST_PATH} --log_level=message
  COMMAND ${CMAKE_COMMAND} -E touch "${unittest_stamp}"
  POST_BUILD
  DEPENDS XCoreUnitTests)
add_custom_target(RUN_CORE_TESTS ALL DEPENDS "${unittest_stamp}")
set_property(TARGET RUN_CORE_TESTS PROPERTY FOLDER "CMakePredefinedTargets")
