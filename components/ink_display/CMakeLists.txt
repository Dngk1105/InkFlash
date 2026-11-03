# Thiết kế: docs/design/display/display.md
idf_component_register(SRCS "ink_display.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_hal ink_display_policy
                       PRIV_REQUIRES ink_frame_slots freertos esp_timer heap)
