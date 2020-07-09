cmake_minimum_required (VERSION 2.8.11)
project(bf)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCES
    main.cpp
)

add_executable(bf ${SOURCES})
