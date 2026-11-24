/* Generated from Figma: SU7kpiOJEQ2DaBnUmDPBff node 69:3 (version 2424764074327803648); hash: sha256:a5fb3b093ccab452 */
/* Design: docs/design/figma-to-code.md */
#include "foundations_2_portrait_view.gen.h"

static const ink_ui_text_style_t s_tiny5_8_start = {
    .font = &ink_ui_font_tiny5_8, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_unifont_vi_16_center = {
    .font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_CENTER, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_unifont_vi_16_end = {
    .font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_END, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_unifont_vi_16_start = {
    .font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_unifont_vi_16_start_gm2 = {
    .font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = -2};
static const ink_ui_text_style_t s_unifont_vi_16_start_l1 = {
    .font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_START, .max_lines = 1, .line_gap = 0};

/* Icon pixels: 13 x 13, MASK1. */
static const uint8_t         s_icon_pixels_data[26] = {0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x88, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x88, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x88};
static const ink_ui_bitmap_t s_icon_pixels = {.width = 13, .height = 13, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_data};
/* Icon pixels: 15 x 15, MASK1. */
static const uint8_t s_icon_pixels_2_data[30] = {0x22, 0x22, 0x00, 0x00, 0xAA, 0xAA, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0xAA, 0xAA, 0x00,
                                                 0x00, 0x22, 0x22, 0x00, 0x00, 0xAA, 0xAA, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0xAA, 0xAA};
static const ink_ui_bitmap_t s_icon_pixels_2  = {
     .width = 15, .height = 15, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_2_data};
/* Icon pixels: 16 x 16, MASK1. */
static const uint8_t         s_icon_pixels_3_data[32] = {0x55, 0x55, 0xEE, 0xEE, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55, 0xEE,
                                                         0xEE, 0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55, 0xEE, 0xEE, 0x55, 0x55,
                                                         0xAA, 0xAA, 0x55, 0x55, 0xEE, 0xEE, 0x55, 0x55, 0xAA, 0xAA};
static const ink_ui_bitmap_t s_icon_pixels_3          = {
             .width = 16, .height = 16, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_3_data};
/* Icon pixels: 16 x 16, MASK1. */
static const uint8_t         s_icon_pixels_4_data[32] = {0x55, 0x55, 0xFF, 0xFF, 0x55, 0x55, 0xFF, 0xFF, 0x55, 0x55, 0xFF,
                                                         0xFF, 0x55, 0x55, 0xFF, 0xFF, 0x55, 0x55, 0xFF, 0xFF, 0x55, 0x55,
                                                         0xFF, 0xFF, 0x55, 0x55, 0xFF, 0xFF, 0x55, 0x55, 0xFF, 0xFF};
static const ink_ui_bitmap_t s_icon_pixels_4          = {
             .width = 16, .height = 16, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_4_data};
/* Icon pixels: 16 x 16, MASK1. */
static const uint8_t         s_icon_pixels_5_data[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x77, 0x77, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                         0xFF, 0x77, 0x77, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x77, 0x77,
                                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x77, 0x77, 0xFF, 0xFF};
static const ink_ui_bitmap_t s_icon_pixels_5          = {
             .width = 16, .height = 16, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_5_data};
/* Icon pixels: 12 x 12, MASK1. */
static const uint8_t         s_icon_pixels_6_data[24] = {0x3F, 0xF0, 0x20, 0x10, 0xFF, 0xD0, 0x80, 0x50, 0x80, 0x50, 0x9E, 0x50,
                                                         0x80, 0x50, 0x9C, 0x50, 0x80, 0x50, 0x80, 0x70, 0x80, 0x40, 0xFF, 0xC0};
static const ink_ui_bitmap_t s_icon_pixels_6          = {
             .width = 12, .height = 12, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_6_data};
/* Icon pixels: 12 x 12, MASK1. */
static const uint8_t         s_icon_pixels_7_data[24] = {0x3F, 0xF0, 0x20, 0x10, 0xFF, 0xD0, 0x80, 0x50, 0x80, 0x50, 0x9E, 0x50,
                                                         0x80, 0x50, 0x9C, 0x50, 0x80, 0x50, 0x80, 0x70, 0x80, 0x40, 0xFF, 0xC0};
static const ink_ui_bitmap_t s_icon_pixels_7          = {
             .width = 12, .height = 12, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = s_icon_pixels_7_data};

void foundations_2_portrait_view_draw(ink_ui_canvas_t *canvas, const foundations_2_portrait_view_data_t *data) {
    (void)data;
    ink_ui_fill(canvas, ink_ui_canvas_bounds(canvas), 0, INK_UI_PAINT_WHITE);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 0, .y = 0, .width = 240, .height = 24}, 0, INK_UI_PAINT_BLACK);
    /* header: Auto Layout row, spacing 88 */
    const ink_ui_length_t header_lengths[2] = {
        {.fixed = 57, .flex = 0},
        {.fixed = 79, .flex = 0}
    };
    ink_ui_rect_t header_slots[2];
    ink_ui_rect_split(ink_ui_rect_inset((ink_ui_rect_t){.x = 0, .y = 0, .width = 240, .height = 24}, 8, 0), INK_UI_AXIS_ROW, 88,
                      header_lengths, 2, header_slots);
    /* FOUNDATIONS 2: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(header_slots[0], (ink_ui_size_t){.width = 57, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "FOUNDATIONS 2", &s_tiny5_8_start, INK_UI_PAINT_WHITE);
    /* ULTRA-LIGHT 240x416: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(header_slots[1], (ink_ui_size_t){.width = 79, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "ULTRA-LIGHT 240x416", &s_tiny5_8_start, INK_UI_PAINT_WHITE);
    /* body: Auto Layout column, spacing 4 */
    const ink_ui_length_t body_lengths[4] = {
        {.fixed = 102, .flex = 0},
        {.fixed = 82,  .flex = 0},
        {.fixed = 114, .flex = 0},
        {.fixed = 30,  .flex = 0}
    };
    ink_ui_rect_t body_slots[4];
    ink_ui_rect_split(ink_ui_rect_inset((ink_ui_rect_t){.x = 0, .y = 24, .width = 240, .height = 392}, 8, 8), INK_UI_AXIS_COLUMN, 4,
                      body_lengths, 4, body_slots);
    /* 06 dither: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_06_dither_lengths[2] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 88, .flex = 0}
    };
    ink_ui_rect_t n_06_dither_slots[2];
    ink_ui_rect_split(body_slots[0], INK_UI_AXIS_COLUMN, 0, n_06_dither_lengths, 2, n_06_dither_slots);
    /* 06 DITHER: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_06_dither_slots[0], (ink_ui_size_t){.width = 84, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "06 DITHER (FILL ONLY)", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* dither list: Auto Layout column, spacing 2 */
    const ink_ui_length_t dither_list_lengths[5] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0}
    };
    ink_ui_rect_t dither_list_slots[5];
    ink_ui_rect_split(
        ink_ui_rect_align(n_06_dither_slots[1], (ink_ui_size_t){.width = 106, .height = 88}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 2, dither_list_lengths, 5, dither_list_slots);
    /* row gray-100: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_gray_100_lengths[3] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 66, .flex = 0}
    };
    ink_ui_rect_t row_gray_100_slots[3];
    ink_ui_rect_split(dither_list_slots[0], INK_UI_AXIS_ROW, 4, row_gray_100_lengths, 3, row_gray_100_slots);
    /* figma gray-100: rectangle 16 x 16 */
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 46, .width = 16, .height = 16}, 0, INK_UI_PAINT_GRAY(244));
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 28, .y = 46, .width = 16, .height = 16}, 0, INK_UI_PAINT_WHITE);
    /* pixels: icon 13 x 13 */
    ink_ui_bitmap(canvas, 28, 47, &s_icon_pixels, INK_UI_PAINT_BLACK);
    /* gray-100 244 15/16: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(row_gray_100_slots[2], (ink_ui_size_t){.width = 66, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "gray-100 244 15/16", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row gray-300: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_gray_300_lengths[3] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 66, .flex = 0}
    };
    ink_ui_rect_t row_gray_300_slots[3];
    ink_ui_rect_split(dither_list_slots[1], INK_UI_AXIS_ROW, 4, row_gray_300_lengths, 3, row_gray_300_slots);
    /* figma gray-300: rectangle 16 x 16 */
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 64, .width = 16, .height = 16}, 0, INK_UI_PAINT_GRAY(212));
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 28, .y = 64, .width = 16, .height = 16}, 0, INK_UI_PAINT_WHITE);
    /* pixels: icon 15 x 15 */
    ink_ui_bitmap(canvas, 28, 65, &s_icon_pixels_2, INK_UI_PAINT_BLACK);
    /* gray-300 212 13/16: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(row_gray_300_slots[2], (ink_ui_size_t){.width = 66, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "gray-300 212 13/16", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row gray-500: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_gray_500_lengths[3] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 62, .flex = 0}
    };
    ink_ui_rect_t row_gray_500_slots[3];
    ink_ui_rect_split(
        ink_ui_rect_align(dither_list_slots[2], (ink_ui_size_t){.width = 102, .height = 16}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_gray_500_lengths, 3, row_gray_500_slots);
    /* figma gray-500: rectangle 16 x 16 */
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 82, .width = 16, .height = 16}, 0, INK_UI_PAINT_GRAY(113));
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 28, .y = 82, .width = 16, .height = 16}, 0, INK_UI_PAINT_WHITE);
    /* pixels: icon 16 x 16 */
    ink_ui_bitmap(canvas, 28, 82, &s_icon_pixels_3, INK_UI_PAINT_BLACK);
    /* gray-500 113 7/16: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(row_gray_500_slots[2], (ink_ui_size_t){.width = 62, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "gray-500 113 7/16", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row gray-700: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_gray_700_lengths[3] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 58, .flex = 0}
    };
    ink_ui_rect_t row_gray_700_slots[3];
    ink_ui_rect_split(
        ink_ui_rect_align(dither_list_slots[3], (ink_ui_size_t){.width = 98, .height = 16}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_gray_700_lengths, 3, row_gray_700_slots);
    /* figma gray-700: rectangle 16 x 16 */
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 100, .width = 16, .height = 16}, 0, INK_UI_PAINT_GRAY(63));
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 28, .y = 100, .width = 16, .height = 16}, 0, INK_UI_PAINT_WHITE);
    /* pixels: icon 16 x 16 */
    ink_ui_bitmap(canvas, 28, 100, &s_icon_pixels_4, INK_UI_PAINT_BLACK);
    /* gray-700 63 4/16: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(row_gray_700_slots[2], (ink_ui_size_t){.width = 58, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "gray-700 63 4/16", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row gray-900: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_gray_900_lengths[3] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 58, .flex = 0}
    };
    ink_ui_rect_t row_gray_900_slots[3];
    ink_ui_rect_split(
        ink_ui_rect_align(dither_list_slots[4], (ink_ui_size_t){.width = 98, .height = 16}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_gray_900_lengths, 3, row_gray_900_slots);
    /* figma gray-900: rectangle 16 x 16 */
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 118, .width = 16, .height = 16}, 0, INK_UI_PAINT_GRAY(24));
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 28, .y = 118, .width = 16, .height = 16}, 0, INK_UI_PAINT_WHITE);
    /* pixels: icon 16 x 16 */
    ink_ui_bitmap(canvas, 28, 118, &s_icon_pixels_5, INK_UI_PAINT_BLACK);
    /* gray-900 24 1/16: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(row_gray_900_slots[2], (ink_ui_size_t){.width = 58, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "gray-900 24 1/16", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* 07 select: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_07_select_lengths[2] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 68, .flex = 0}
    };
    ink_ui_rect_t n_07_select_slots[2];
    ink_ui_rect_split(body_slots[1], INK_UI_AXIS_COLUMN, 0, n_07_select_lengths, 2, n_07_select_slots);
    /* 07 SELECT: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_07_select_slots[0], (ink_ui_size_t){.width = 39, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "07 SELECT", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* select list: Auto Layout column, spacing 2 */
    const ink_ui_length_t select_list_lengths[3] = {
        {.fixed = 20, .flex = 0},
        {.fixed = 20, .flex = 0},
        {.fixed = 24, .flex = 0}
    };
    ink_ui_rect_t select_list_slots[3];
    ink_ui_rect_split(
        ink_ui_rect_align(n_07_select_slots[1], (ink_ui_size_t){.width = 99, .height = 68}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 2, select_list_lengths, 3, select_list_slots);
    /* item normal: Auto Layout row, spacing 0 */
    const ink_ui_length_t item_normal_lengths[2] = {
        {.fixed = 56, .flex = 0},
        {.fixed = 25, .flex = 0}
    };
    ink_ui_rect_t item_normal_slots[2];
    ink_ui_rect_split(ink_ui_rect_inset(select_list_slots[0], 4, 2), INK_UI_AXIS_ROW, 0, item_normal_lengths, 2, item_normal_slots);
    /* label: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, item_normal_slots[0], "Thẻ mới", &s_unifont_vi_16_start, INK_UI_PAINT_BLACK);
    /* kind: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_normal_slots[1], (ink_ui_size_t){.width = 25, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        " normal", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 174, .width = 99, .height = 20}, 0, INK_UI_PAINT_BLACK);
    /* item inverse: Auto Layout row, spacing 0 */
    const ink_ui_length_t item_inverse_lengths[2] = {
        {.fixed = 56, .flex = 0},
        {.fixed = 27, .flex = 0}
    };
    ink_ui_rect_t item_inverse_slots[2];
    ink_ui_rect_split(ink_ui_rect_inset(select_list_slots[1], 4, 2), INK_UI_AXIS_ROW, 0, item_inverse_lengths, 2, item_inverse_slots);
    /* label: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, item_inverse_slots[0], "Thẻ mới", &s_unifont_vi_16_start, INK_UI_PAINT_WHITE);
    /* kind: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_inverse_slots[1], (ink_ui_size_t){.width = 27, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        " inverse", &s_tiny5_8_start, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 196, .width = 99, .height = 24}, 2, 2, INK_UI_PAINT_BLACK);
    /* item outline 2: Auto Layout row, spacing 0 */
    const ink_ui_length_t item_outline_2_lengths[2] = {
        {.fixed = 56, .flex = 0},
        {.fixed = 31, .flex = 0}
    };
    ink_ui_rect_t item_outline_2_slots[2];
    ink_ui_rect_split((ink_ui_rect_t){.x = 14, .y = 198, .width = 87, .height = 20}, INK_UI_AXIS_ROW, 0, item_outline_2_lengths, 2,
                      item_outline_2_slots);
    /* label: static text, unifont_vi_16 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_outline_2_slots[0], (ink_ui_size_t){.width = 56, .height = 16}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "Thẻ mới", &s_unifont_vi_16_start, INK_UI_PAINT_BLACK);
    /* kind: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_outline_2_slots[1], (ink_ui_size_t){.width = 31, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        " outline 2", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* 08 text: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_08_text_lengths[2] = {
        {.fixed = 14,  .flex = 0},
        {.fixed = 100, .flex = 0}
    };
    ink_ui_rect_t n_08_text_slots[2];
    ink_ui_rect_split(body_slots[2], INK_UI_AXIS_COLUMN, 0, n_08_text_lengths, 2, n_08_text_slots);
    /* 08 TEXT: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_08_text_slots[0], (ink_ui_size_t){.width = 29, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "08 TEXT", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* text list: Auto Layout column, spacing 2 */
    const ink_ui_length_t text_list_lengths[5] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 28, .flex = 0}
    };
    ink_ui_rect_t text_list_slots[5];
    ink_ui_rect_split(
        ink_ui_rect_align(n_08_text_slots[1], (ink_ui_size_t){.width = 104, .height = 100}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 2, text_list_lengths, 5, text_list_slots);
    /* align left: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, text_list_slots[0], "Trái", &s_unifont_vi_16_start, INK_UI_PAINT_BLACK);
    /* align center: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, text_list_slots[1], "Giữa", &s_unifont_vi_16_center, INK_UI_PAINT_BLACK);
    /* align right: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, text_list_slots[2], "Phải", &s_unifont_vi_16_end, INK_UI_PAINT_BLACK);
    /* max_lines 1: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, text_list_slots[3], "Một câu rất dài bị cắt bớt", &s_unifont_vi_16_start_l1, INK_UI_PAINT_BLACK);
    /* line_gap -2: static text, unifont_vi_16 */
    {
        const ink_ui_rect_t text_rect =
            ink_ui_rect_align(text_list_slots[4], (ink_ui_size_t){.width = 64, .height = 28}, INK_UI_ALIGN_START, INK_UI_ALIGN_START);
        ink_ui_text_draw(canvas,
                         (ink_ui_rect_t){.x      = text_rect.x,
                                         .y      = (int16_t)(text_rect.y + (-1)),
                                         .width  = text_rect.width,
                                         .height = (int16_t)(text_rect.height - (-2))},
                         "Dòng một\nDòng hai", &s_unifont_vi_16_start_gm2, INK_UI_PAINT_BLACK);
    }
    /* 09 icon: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_09_icon_lengths[2] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 16, .flex = 0}
    };
    ink_ui_rect_t n_09_icon_slots[2];
    ink_ui_rect_split(body_slots[3], INK_UI_AXIS_COLUMN, 0, n_09_icon_lengths, 2, n_09_icon_slots);
    /* 09 ICON: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_09_icon_slots[0], (ink_ui_size_t){.width = 60, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "09 ICON (MASK1)", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* icon list: Auto Layout row, spacing 8 */
    const ink_ui_length_t icon_list_lengths[2] = {
        {.fixed = 37, .flex = 0},
        {.fixed = 45, .flex = 0}
    };
    ink_ui_rect_t icon_list_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(n_09_icon_slots[1], (ink_ui_size_t){.width = 90, .height = 16}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 8, icon_list_lengths, 2, icon_list_slots);
    /* pair paint: Auto Layout row, spacing 4 */
    const ink_ui_length_t pair_paint_lengths[2] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 17, .flex = 0}
    };
    ink_ui_rect_t pair_paint_slots[2];
    ink_ui_rect_split(icon_list_slots[0], INK_UI_AXIS_ROW, 4, pair_paint_lengths, 2, pair_paint_slots);
    /* pixels: icon 12 x 12 */
    ink_ui_bitmap(canvas, 10, 357, &s_icon_pixels_6, INK_UI_PAINT_BLACK);
    /* paint: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(pair_paint_slots[1], (ink_ui_size_t){.width = 17, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "paint", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* pair inverse: Auto Layout row, spacing 4 */
    const ink_ui_length_t pair_inverse_lengths[2] = {
        {.fixed = 16, .flex = 0},
        {.fixed = 25, .flex = 0}
    };
    ink_ui_rect_t pair_inverse_slots[2];
    ink_ui_rect_split(icon_list_slots[1], INK_UI_AXIS_ROW, 4, pair_inverse_lengths, 2, pair_inverse_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 53, .y = 356, .width = 16, .height = 16}, 0, INK_UI_PAINT_BLACK);
    /* pixels: icon 12 x 12 */
    ink_ui_bitmap(canvas, 55, 357, &s_icon_pixels_7, INK_UI_PAINT_WHITE);
    /* inverse: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(pair_inverse_slots[1], (ink_ui_size_t){.width = 25, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "inverse", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
}
