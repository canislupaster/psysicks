project(psysicks)

cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)
project(psysicks)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(BUILD_DEBUG)
endif ()

add_subdirectory(corecommon)
include_directories(./corecommon/src)

find_package(PkgConfig REQUIRED)

add_subdirectory(./fem)
add_subdirectory(./parser)
add_subdirectory(./plotter)
