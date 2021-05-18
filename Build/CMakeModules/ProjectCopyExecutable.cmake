
# copy executable targets into the BINARY_DIR dir
function(project_copy_executable TARGET_NAME)
  set(options OPTIONAL DEST_PATH)
  # strip leading X from project name
  string(REGEX REPLACE "^X(.+)$" "\\1" EXECUTABLE_NAME ${TARGET_NAME})
  # use runtime CMAKE_CFG_INTDIR in Visual CPP and XCode projects, else the static CMAKE_BUILD_TYPE
  if("${CMAKE_GENERATOR}" MATCHES "Visual Studio" OR "${CMAKE_GENERATOR}" MATCHES "Xcode")
    set(TARGET_PATH ${CMAKE_SOURCE_DIR}/../Dist/${CMAKE_CFG_INTDIR})
  else()
    set(TARGET_PATH ${CMAKE_SOURCE_DIR}/../Dist/${CMAKE_BUILD_TYPE})
  endif()
  set(TARGET_FILE_PATH ${TARGET_PATH}/${EXECUTABLE_NAME}${CMAKE_EXECUTABLE_SUFFIX})
  # copy exe into dist dir as POST_BUILD step
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${TARGET_PATH}
      COMMAND ${CMAKE_COMMAND} -E echo \"copy $<TARGET_FILE:${TARGET_NAME}> -> ${TARGET_FILE_PATH}\"
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_NAME}> ${TARGET_FILE_PATH})
  # set dest path
  if(ARGV1)
    set(${ARGV1} ${TARGET_FILE_PATH} PARENT_SCOPE)
  endif()
endfunction(project_copy_executable)
