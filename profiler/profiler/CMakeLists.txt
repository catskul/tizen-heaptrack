cmake_minimum_required(VERSION 2.8.12.2)

project(profiler)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -g -std=c++11")

add_subdirectory(
  src
)

