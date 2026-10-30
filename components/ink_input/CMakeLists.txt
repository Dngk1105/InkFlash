# Thiết kế: docs/design/input.md
idf_component_register(SRCS "ink_input.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_gesture ink_hal
                       PRIV_REQUIRES ink_platform)
