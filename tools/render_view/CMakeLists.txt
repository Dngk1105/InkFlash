# Design: docs/design/figma-to-code.md
#
# Host renderer: build one view (.gen.c) with ink_ui and a main.c written by tools/render_view.py, using gcc on the PC.
# Do not run by hand; tools/render_view.py configures it with:
#   RENDER_MAIN  path of the generated main.c
#   VIEW_SOURCE  path of the view .gen.c (its directory is added to the include path)
cmake_minimum_required(VERSION 3.22)
project(inkflash_render_view C)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT RENDER_MAIN OR NOT VIEW_SOURCE)
    message(FATAL_ERROR "Run through tools/render_view.py (needs RENDER_MAIN and VIEW_SOURCE)")
endif()

# Same pure C components as test/host, without their tests.
get_filename_component(INK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
file(GLOB_RECURSE INK_COMPONENT_LISTS "${INK_ROOT}/components/*/CMakeLists.txt")
foreach(list_file ${INK_COMPONENT_LISTS})
    file(READ "${list_file}" content)
    string(FIND "${content}" "ink_core_component(" core_pos)
    string(FIND "${content}" "ink_third_party_component(" third_party_pos)
    if(core_pos EQUAL -1 AND third_party_pos EQUAL -1)
        continue()
    endif()
    get_filename_component(dir "${list_file}" DIRECTORY)
    get_filename_component(name "${dir}" NAME)
    add_subdirectory("${dir}" "${CMAKE_BINARY_DIR}/${name}")
endforeach()

get_filename_component(VIEW_DIR "${VIEW_SOURCE}" DIRECTORY)
add_executable(render_view "${RENDER_MAIN}" "${VIEW_SOURCE}")
target_include_directories(render_view PRIVATE "${VIEW_DIR}")
target_link_libraries(render_view PRIVATE ink_ui)
target_compile_options(render_view PRIVATE -Wall -Wextra -Werror -pedantic)
