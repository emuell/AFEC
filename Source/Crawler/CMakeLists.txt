# allow including project internal libraries and Core projects 
include_directories(.)
include_directories(../Core)

# include sub project CMakeLists
add_subdirectory(Classification)
add_subdirectory(FeatureExtraction)

add_subdirectory(XCrawler)

if(BUILD_TESTS)
  add_subdirectory(XModelTester)
  add_subdirectory(XModelCreator)
  add_subdirectory(XUnitTests)
endif()
