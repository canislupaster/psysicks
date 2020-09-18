file(GLOB PARSERSRC ./*.c)
add_executable(parser ${PARSERSRC})

add_custom_target(genheader_parser COMMAND headergen ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(parser genheader_parser corecommon)
target_link_libraries(parser PUBLIC corecommon)