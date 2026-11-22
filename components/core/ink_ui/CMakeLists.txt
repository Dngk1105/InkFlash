# Design: docs/design/ui.md
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/ink.cmake)
ink_core_component(SRCS "ink_ui_canvas.c" "ink_ui_text.c" "ink_ui_layout.c"
                        "fonts/ink_ui_font_unifont_vi_16.c" "fonts/ink_ui_font_fixed_10.c"
                        "fonts/ink_ui_font_tiny5_8.c" "fonts/ink_ui_font_tiny5_16.c" "fonts/ink_ui_font_tiny5_24.c"
                   REQUIRES ink_display_policy ink_u8g2)
