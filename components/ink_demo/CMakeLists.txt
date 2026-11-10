# Design: docs/design/ui.md
idf_component_register(SRCS "ink_demo.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_gesture
                       PRIV_REQUIRES freertos ink_display ink_platform ink_ui)
