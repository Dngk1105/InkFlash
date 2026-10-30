# Thiết kế: docs/design/input/input.md
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/ink.cmake)
ink_core_component(SRCS "ink_keymap.c" REQUIRES ink_gesture)
