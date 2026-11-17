/* Generated from Figma: none (hand-written sample of the view generator output); hash: none */
/* Design: docs/design/figma-to-code.md */
#include "example_view.gen.h"

/* Screen frame 240 x 416, children placed freely. */
static const ink_ui_rect_t s_header = {.x = 0, .y = 0, .width = 240, .height = 40};
static const ink_ui_rect_t s_body   = {.x = 0, .y = 40, .width = 240, .height = 328};
static const ink_ui_rect_t s_band   = {.x = 0, .y = 368, .width = 240, .height = 8};
static const ink_ui_rect_t s_footer = {.x = 0, .y = 376, .width = 240, .height = 40};

/* Text style unifont_vi_16, one line. */
static const ink_ui_text_style_t s_unifont_center = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_CENTER, .max_lines = 1};
static const ink_ui_text_style_t s_unifont_start  = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_START, .max_lines = 1};

/* Text style fixed_10, one line. */
static const ink_ui_text_style_t s_fixed_center = {.font = &ink_ui_font_fixed_10, .align = INK_UI_ALIGN_CENTER, .max_lines = 1};

void example_view_draw_header(ink_ui_canvas_t *canvas, const example_view_data_t *data) {
    ink_ui_fill(canvas, s_header, 0, INK_UI_PAINT_BLACK);
    /* {title}: fixed 224 x 16 at (8, 12) */
    ink_ui_text_draw(canvas, (ink_ui_rect_t){.x = 8, .y = 12, .width = 224, .height = 16}, data->title, &s_unifont_center,
                     INK_UI_PAINT_WHITE);
}

void example_view_draw_body(ink_ui_canvas_t *canvas, const example_view_data_t *data) {
    ink_ui_fill(canvas, s_body, 0, INK_UI_PAINT_WHITE);
    /* Auto Layout column, padding 8 / 8, spacing 8: *rows fixed 128, ?empty_hint fixed 16 */
    const ink_ui_length_t lengths[2] = {
        {.fixed = 128, .flex = 0},
        {.fixed = 16,  .flex = 0}
    };
    ink_ui_rect_t slots[2];
    ink_ui_rect_split(ink_ui_rect_inset(s_body, 8, 8), INK_UI_AXIS_COLUMN, 8, lengths, 2, slots);

    /* *rows: Auto Layout column, spacing 0, rows fixed 32 */
    const ink_ui_length_t row_lengths[EXAMPLE_VIEW_ROWS_MAX] = {{.fixed = 32}, {.fixed = 32}, {.fixed = 32}, {.fixed = 32}};
    ink_ui_rect_t         rows[EXAMPLE_VIEW_ROWS_MAX];
    ink_ui_rect_split(slots[0], INK_UI_AXIS_COLUMN, 0, row_lengths, EXAMPLE_VIEW_ROWS_MAX, rows);
    const uint8_t count = data->rows_count < EXAMPLE_VIEW_ROWS_MAX ? data->rows_count : EXAMPLE_VIEW_ROWS_MAX;
    for (uint8_t i = 0; i < count; i++) {
        const ink_ui_rect_t row = rows[i];
        /* {label}: fixed, padding 8 / 8 */
        ink_ui_text_draw(canvas, ink_ui_rect_inset(row, 8, 8), data->rows[i].label, &s_unifont_start, INK_UI_PAINT_BLACK);
        /* Line at the bottom edge of the row, weight 1 */
        ink_ui_line(canvas, row.x, (int16_t)(row.y + row.height - 1), (int16_t)(row.x + row.width - 1), (int16_t)(row.y + row.height - 1),
                    1, INK_UI_PAINT_BLACK);
    }

    if (data->show_empty_hint) {
        /* ?empty_hint: static text */
        ink_ui_text_draw(canvas, slots[1], "Nhấn A để thêm hàng", &s_unifont_center, INK_UI_PAINT_BLACK);
    }
}

void example_view_draw(ink_ui_canvas_t *canvas, const example_view_data_t *data) {
    ink_ui_fill(canvas, ink_ui_canvas_bounds(canvas), 0, INK_UI_PAINT_WHITE);
    example_view_draw_header(canvas, data);
    example_view_draw_body(canvas, data);

    /* Band: gray fill R = 128 */
    ink_ui_fill(canvas, s_band, 0, INK_UI_PAINT_GRAY(128));

    /* Footer: white fill, line weight 1 on top, static text fixed_10 */
    ink_ui_fill(canvas, s_footer, 0, INK_UI_PAINT_WHITE);
    ink_ui_line(canvas, s_footer.x, s_footer.y, (int16_t)(s_footer.x + s_footer.width - 1), s_footer.y, 1, INK_UI_PAINT_BLACK);
    ink_ui_text_draw(canvas, (ink_ui_rect_t){.x = 8, .y = 391, .width = 224, .height = 10}, "A: add   B: reset", &s_fixed_center,
                     INK_UI_PAINT_BLACK);
}
