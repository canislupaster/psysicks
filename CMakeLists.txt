cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)
project(fps)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(BUILD_DEBUG)
endif ()

add_subdirectory(corecommon)
include_directories(./corecommon/src ./cgltf ./lodepng)

file(GLOB FPSSRC ./src/*.c)
file(GLOB UTILSRC ./src/*.c)

list(REMOVE_ITEM FPSSRC ./src/util_main.c)
list(REMOVE_ITEM UTILSRC ./src/main.c)

add_executable(fps ${FPSSRC})
add_executable(util ${UTILSRC})

find_package(OpenGL REQUIRED)
find_package(SDL REQUIRED)
find_package(Freetype REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CGLM REQUIRED cglm)

add_custom_target(genheader_fps COMMAND ${CMAKE_CURRENT_BINARY_DIR}/corecommon/headergen ${CMAKE_SOURCE_DIR}/src)
add_dependencies(fps genheader_fps corecommon)

target_include_directories(fps PUBLIC ${SDL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${CGLM_INCLUDE_DIRS})
target_link_libraries(fps PUBLIC corecommon ${OPENGL_LIBRARY} ${SDL_LIBRARY} ${FREETYPE_LIBRARY})

target_include_directories(util PUBLIC ${SDL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${CGLM_INCLUDE_DIRS})
target_link_libraries(util PUBLIC corecommon ${OPENGL_LIBRARY} ${SDL_LIBRARY} ${FREETYPE_LIBRARY})
