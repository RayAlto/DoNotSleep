cmake_minimum_required(VERSION 3.8...3.27)
project(
  do-not-sleep
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "Write random data periodically to storage devices that cannot be disabled from sleeping")
option(BUILD_TESTS "Build sources in `/test` directory" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

set(${CMAKE_PROJECT_NAME}_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${CMAKE_PROJECT_NAME}_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/block_info.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ds.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/hms.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/util.cc)

add_executable(${CMAKE_PROJECT_NAME} ${${CMAKE_PROJECT_NAME}_SRCS})
target_compile_features(${CMAKE_PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${${CMAKE_PROJECT_NAME}_INCLUDES})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${JSONCPP_LIBRARIES})
