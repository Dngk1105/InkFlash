# Thiết kế: docs/design/code-structure.md
#
# Project ESP-IDF riêng chứa khối mẫu. Không nằm trong firmware; tools/build_all.py build nó để template không bị cũ.
cmake_minimum_required(VERSION 3.22)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/ink.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
ink_project_flags()
project(inkflash_templates)
