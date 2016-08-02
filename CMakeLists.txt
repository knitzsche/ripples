cmake_minimum_required(VERSION 2.6)
project(ripples)

# Use our modified FindSDL2* modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ripples_SOURCE_DIR}/cmake")
message("mod path " ${CMAKE_MODULE_PATH})
set(BIN_DIR ${games_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11")

find_package(SDL2 REQUIRED)
include(FindPkgConfig)
include_directories(${SDL2_INCLUDE_DIR})
include_directories(include)

add_subdirectory(game1)

