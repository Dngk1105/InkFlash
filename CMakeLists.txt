# Thiết kế: docs/design/code-structure.md
cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/ink.cmake)
ink_extra_component_dirs(components/core components/drivers components/apps)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
ink_project_flags()
project(inkflash)
ink_fix_clangd()
