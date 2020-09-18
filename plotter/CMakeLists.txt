file(GLOB PLOTTERSRC ./*.c)
add_executable(plotter ${PLOTTERSRC})

find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

pkg_check_modules(CGLM REQUIRED cglm)
pkg_check_modules(SDL REQUIRED sdl2)

add_custom_target(genheader_plotter COMMAND headergen ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(plotter genheader_plotter corecommon)

target_include_directories(plotter PUBLIC ${SDL_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${CGLM_INCLUDE_DIRS} ./cgltf)
target_link_libraries(plotter PUBLIC corecommon ${OPENGL_LIBRARY} ${SDL_LIBRARY} ${FREETYPE_LIBRARY})
