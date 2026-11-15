# Design: docs/design/ui.md
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/ink.cmake)
# Only the drawing core of U8g2. Not compiled: u8g2_fonts.c (39.7 MB; fonts are copied one by one with
# tools/u8g2_font.py) and the display drivers (u8x8_d_*.c, u8g2_d_*.c).
ink_third_party_component(SRCS "ink_u8g2.c"
                          THIRD_PARTY_DIR third_party/u8g2/csrc
                          THIRD_PARTY_SRCS u8g2_bitmap.c u8g2_box.c u8g2_buffer.c u8g2_circle.c u8g2_font.c u8g2_hvline.c
                                           u8g2_intersection.c u8g2_kerning.c u8g2_line.c u8g2_ll_hvline.c u8g2_setup.c
                                           u8x8_8x8.c u8x8_byte.c u8x8_cad.c u8x8_display.c u8x8_gpio.c u8x8_setup.c)
