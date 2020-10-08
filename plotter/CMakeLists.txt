file(GLOB PLOTTERSRC ./*.c)
add_executable(plotter ${PLOTTERSRC})

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)

pkg_check_modules(CGLM REQUIRED cglm)

add_custom_target(genheader_plotter WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND headergen ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(plotter genheader_plotter corecommon)

target_include_directories(plotter PUBLIC ${SDL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${CGLM_INCLUDE_DIRS} ./cgltf)
target_link_libraries(plotter PUBLIC corecommon ${OPENGL_LIBRARY} ${SDLMAIN_LIBRARY} ${FREETYPE_LIBRARY})
