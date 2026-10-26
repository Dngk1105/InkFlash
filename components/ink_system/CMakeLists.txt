# Thiết kế: docs/design/input.md
idf_component_register(SRCS "ink_system.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_hal
                       PRIV_REQUIRES freertos ink_gesture ink_platform)
