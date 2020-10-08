file(GLOB FEMSRC ./*.c)
add_executable(fem ${FEMSRC})

add_custom_target(genheader_fem COMMAND headergen ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(fem genheader_fem corecommon)
target_link_libraries(fem PUBLIC corecommon)
