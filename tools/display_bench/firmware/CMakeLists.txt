# Thiết kế: docs/design/display/bench.md
#
# Firmware test màn: project ESP-IDF riêng, dùng lại component của firmware chính, không nằm trong firmware chính.
cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/ink.cmake)
set(EXTRA_COMPONENT_DIRS
    "${INK_ROOT_DIR}/components"
    "${INK_ROOT_DIR}/components/core"
    "${INK_ROOT_DIR}/components/drivers")
set(COMPONENTS main) # chỉ build main và các component nó cần
set(SDKCONFIG_DEFAULTS "${INK_ROOT_DIR}/sdkconfig.defaults;${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
ink_project_flags()
project(display_bench)
ink_fix_clangd()
