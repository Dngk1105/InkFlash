# Thiết kế: docs/design/code-structure.md
#
# Host test: build mọi khối C thuần (component gọi ink_core_component) bằng gcc trên PC, kèm test trong <khối>/test/.
# Unity lấy từ mã nguồn có sẵn trong ESP-IDF, nên cần biến môi trường IDF_PATH.
cmake_minimum_required(VERSION 3.22)
project(inkflash_host C)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF) # -std=c23, không phần mở rộng GNU
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # clangd đọc build-host/compile_commands.json cho file */test/*.c

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "Cần biến môi trường IDF_PATH (Unity lấy từ ESP-IDF)")
endif()
file(TO_CMAKE_PATH "$ENV{IDF_PATH}" IDF_PATH)
add_library(unity STATIC "${IDF_PATH}/components/unity/unity/src/unity.c")
target_include_directories(unity PUBLIC "${IDF_PATH}/components/unity/unity/src")

get_filename_component(INK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
file(GLOB_RECURSE INK_COMPONENT_LISTS
    "${INK_ROOT}/components/*/CMakeLists.txt"
    "${INK_ROOT}/templates/components/*/CMakeLists.txt")

enable_testing()
foreach(list_file ${INK_COMPONENT_LISTS})
    file(READ "${list_file}" content)
    string(FIND "${content}" "ink_core_component(" pos)
    if(pos EQUAL -1)
        continue()
    endif()
    get_filename_component(dir "${list_file}" DIRECTORY)
    get_filename_component(name "${dir}" NAME)
    add_subdirectory("${dir}" "${CMAKE_BINARY_DIR}/${name}")
    file(GLOB tests "${dir}/test/*.c")
    foreach(test_src ${tests})
        get_filename_component(test_name "${test_src}" NAME_WE)
        add_executable(${test_name} "${test_src}")
        target_link_libraries(${test_name} PRIVATE ${name} unity)
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Werror -pedantic)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endforeach()
