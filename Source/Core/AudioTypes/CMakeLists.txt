set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAudioTypes")

include_directories(../../../3rdParty/Boost/Dist)
include_directories(../../../3rdParty/Resample/Dist/include)
if(WITH_INTEL_IPP)
  include_directories(../../../3rdParty/IPP/Dist)
endif()

include_directories(Source)

project_source_files(PROJECT_SOURCE_FILES)
add_library(AudioTypes ${PROJECT_SOURCE_FILES})
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	add_precompiled_header(
		AudioTypes 
		AudioTypesPrecompiledHeader.h 
		SOURCE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/Source/AudioTypesPrecompiledHeader.cpp)
endif()

set_property(TARGET AudioTypes PROPERTY FOLDER "Core")