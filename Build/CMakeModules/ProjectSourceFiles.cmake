
# collect project cpp/h files
function(project_source_files RETURN_VALUE_NAME)
  message(STATUS "Processing: ${CMAKE_CURRENT_SOURCE_DIR}")

  # add export headers (just for the VS project FILES)
  file(GLOB_RECURSE EXPORT_H
      "${CMAKE_CURRENT_SOURCE_DIR}/Export/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/Export/*.inl")
  list(APPEND FILES ${EXPORT_H})
  source_group("Export" FILES ${EXPORT_H})

  # collect non platform specific source FILES
  file(GLOB_RECURSE COMMON_CPP
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.inl"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")
  source_group("Source" FILES ${COMMON_CPP})

  # collect platform specific source FILES
  file(GLOB_RECURSE MAC_CPP
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Mac*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Mac*.mm"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Mac*.cpp")
  list(LENGTH MAC_CPP MAC_CPP_LENGTH)
  if(${MAC_CPP_LENGTH} GREATER 0)
    list(REMOVE_ITEM COMMON_CPP ${MAC_CPP})
  endif()
  source_group("Source" FILES ${MAC_CPP})

  file(GLOB_RECURSE LINUX_CPP
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Linux*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Linux*.inl"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Linux*.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Linux*.cpp")
  list(LENGTH LINUX_CPP LINUX_CPP_LENGTH)
  if(${LINUX_CPP_LENGTH} GREATER 0)
    list(REMOVE_ITEM COMMON_CPP ${LINUX_CPP})
  endif()
  source_group("Source" FILES ${LINUX_CPP})

  file(GLOB_RECURSE WIN_CPP
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Win*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Win*.inl"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Win*.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/Source/Win*.cpp")
  list(LENGTH WIN_CPP WIN_CPP_LENGTH)
  if(${WIN_CPP_LENGTH} GREATER 0)
    list(REMOVE_ITEM COMMON_CPP ${WIN_CPP})
  endif()
  source_group("Source" FILES ${WIN_CPP})

  # add common and this platform's cpp FILES
  list(APPEND FILES ${COMMON_CPP})

  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND FILES ${LINUX_CPP})
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND FILES ${MAC_CPP})
    list(APPEND FILES ${MAC_MM})
  elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(APPEND FILES ${WIN_CPP})
  else()
    message(FATAL_ERROR "Unexpected or unsupported CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
  endif()

  # add MSVC natvis files (debugger visualization files)
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    file(GLOB_RECURSE MSVC_NATVIS
      "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.natvis")
    list(APPEND FILES ${MSVC_NATVIS})
    source_group("Resources" FILES ${MSVC_NATVIS})
  endif()
  
  # add test files (when building tests)
  if(BUILD_TESTS)
    file(GLOB TEST_H "${CMAKE_CURRENT_SOURCE_DIR}/Test/*.h")
    list(APPEND FILES ${TEST_H})
    file(GLOB TEST_CPP "${CMAKE_CURRENT_SOURCE_DIR}/Test/*.cpp")
    list(APPEND FILES ${TEST_CPP})
    source_group("Test" FILES ${TEST_H} ${TEST_CPP})
  endif(BUILD_TESTS)

  # sort and return
  list(SORT FILES)
  set(${RETURN_VALUE_NAME} ${FILES} PARENT_SCOPE)
endfunction(project_source_files)
