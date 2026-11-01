# Thiết kế: docs/design/display/display.md
idf_component_register(SRCS "ink_display.c" "ink_display_console.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_hal
                       PRIV_REQUIRES ink_display_policy ink_platform freertos esp_timer heap)
