set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMCoreTypes")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/Iconv/Dist/include)
include_directories(../../../3rdParty/ZLib/Dist)
if(WITH_INTEL_IPP)
  include_directories(../../../3rdParty/IPP/Dist)
endif()

include_directories(Source)

project_source_files(PROJECT_SOURCE_FILES)
add_library(CoreTypes ${PROJECT_SOURCE_FILES})
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	add_precompiled_header(
		CoreTypes 
		CoreTypesPrecompiledHeader.h 
		SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/Source/CoreTypesPrecompiledHeader.cpp)
endif()

set_property(TARGET CoreTypes PROPERTY FOLDER "Core")
