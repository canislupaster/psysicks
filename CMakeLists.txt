cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)
project(fps)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(BUILD_DEBUG)
endif ()

add_subdirectory(corecommon)
include_directories(./corecommon/src)

file(GLOB SRC ./src/*.c)

add_executable(fps ${SRC})
add_dependencies(fps corecommon)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(LIBPNG16 REQUIRED IMPORTED_TARGET libpng16)

pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua-5.1)

target_link_libraries(fps PUBLIC corecommon PkgConfig::CAIRO PkgConfig::LIBPNG16 PkgConfig::LUA)