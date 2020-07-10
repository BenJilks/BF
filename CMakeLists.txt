cmake_minimum_required (VERSION 2.8.11)
project(bf)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(bfc compiler.cpp)
add_executable(bf interpreter.cpp)

