set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMClassification")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/LibXtract/Dist/include)
include_directories(../../../3rdParty/OpenBLAS/Dist)
include_directories(../../../3rdParty/Shark/Dist/include)
include_directories(../../../3rdParty/TinyDNN/Dist)
include_directories(../../../3rdParty/LightGBM/Dist/include)

include_directories(Source)

project_source_files(PROJECT_SOURCE_FILES)
add_library(Classification ${PROJECT_SOURCE_FILES})
set_property(TARGET Classification PROPERTY FOLDER "Crawler")
