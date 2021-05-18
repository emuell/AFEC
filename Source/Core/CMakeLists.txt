# allow including project internal libraries only 
include_directories(.)

# include sub project CMakeLists
add_subdirectory(AudioTypes)
add_subdirectory(CoreFileFormats)
add_subdirectory(CoreTypes)

if(BUILD_TESTS)
  add_subdirectory(XCoreUnitTests)
endif()
