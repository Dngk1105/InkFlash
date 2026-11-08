# Design: docs/design/code-structure.md
#
# Separate ESP-IDF project holding the template blocks. Not part of the firmware; tools/build_all.py builds it so templates stay current.
cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/ink.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
ink_project_flags()
project(inkflash_templates)
ink_fix_clangd()
