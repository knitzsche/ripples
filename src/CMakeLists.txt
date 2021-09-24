cmake_minimum_required(VERSION 2.6)
project(ripples)

set(CMAKE_MODULE_PATH "${ripples_SOURCE_DIR}/cmake")
#message("mod path " ${CMAKE_MODULE_PATH})
set(BIN_DIR ${games_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11 -s")
find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
include(FindPkgConfig)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(include)

add_subdirectory(game1)

