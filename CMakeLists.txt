cmake_minimum_required (VERSION 2.6)
project (dataSourceTest)


# The version number.
set (dataSourceTest_VERSION_MAJOR 1)
set (dataSourceTest_VERSION_MINOR 0)


# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/dataSourceTestConfig.h.in"
  "${PROJECT_BINARY_DIR}/dataSourceTestConfig.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find dataSourceTestConfig.h
include_directories( ${PROJECT_BINARY_DIR} include )

set(SOURCES src/main.cpp src/dataSource.cpp)

add_executable(dataSourceTest ${SOURCES})

target_link_libraries(dataSourceTest LINK_PUBLIC Ws2_32 )

