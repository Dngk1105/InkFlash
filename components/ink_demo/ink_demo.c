/* Design: docs/design/ui.md */
#include "ink_demo.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_display_frame.h"
#include "ink_platform.h"
#include "ink_ui.h"

static const char *TAG = "ink_demo";

#define SCREEN_COUNT 4
#define GRID_CELLS   6  /* cells of the ruler screen: 2 columns × 3 rows */
#define PHASE_GAP_MS 20 /* wait between the two frames of the two-phase screen; see screen_two_phase() */

static ink_ui_canvas_t s_canvas;
static bool            s_started;
static uint8_t         s_screen;   /* 0 .. SCREEN_COUNT - 1 */
static uint8_t         s_selected; /* highlighted cell of the ruler screen; moves on every visit */

/* ---- Leaf functions ---- */

static const char *s_sample_paragraph = "Máy học flashcard bỏ túi InkFlash sử dụng vi điều khiển ESP32-S3, màn hình mực điện tử e-ink độ "
                                        "phân giải cao và phím bấm điều hướng rất tiện lợi.";

static const uint8_t s_arrow_bitmap_data[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x30, 0x00, 0x38, 0x00, 0x3C, 0xFF, 0xFE, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x3C, 0x00, 0x38, 0x00, 0x30, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
};

static const ink_ui_bitmap_t s_arrow_bitmap = {
    .width  = 16,
    .height = 16,
    .format = INK_UI_BITMAP_FORMAT_MASK1,
    .data   = s_arrow_bitmap_data,
};

/* +1 for PRESS of button A without modifier, -1 for PRESS of button B without modifier, 0 for anything else. */
static int8_t screen_step(const ink_gesture_t *gesture) {
    if (gesture == nullptr) { return 0; }
    if (gesture->source == INK_GESTURE_SOURCE_BUTTON
        && gesture->type == INK_GESTURE_TYPE_PRESS
        && gesture->modifier == INK_GESTURE_MODIFIER_NONE) {
        if (gesture->button == INK_GESTURE_BUTTON_A) { return 1; }
        if (gesture->button == INK_GESTURE_BUTTON_B) { return -1; }
    }
    return 0;
}

/* Clear the canvas to white, draw a title bar (black bar, white text `title`) at the top and return the area below
 * it, inset by 8 px. */
static ink_ui_rect_t draw_page(const char *title) {
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(&s_canvas);
    ink_ui_fill(&s_canvas, bounds, 0, INK_UI_PAINT_WHITE);

    const int16_t       title_height = 24;
    const ink_ui_rect_t bar          = {.x = 0, .y = 0, .width = bounds.width, .height = title_height};
    ink_ui_fill(&s_canvas, bar, 0, INK_UI_PAINT_BLACK);

    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_CENTER,
        .max_lines = 1,
        .line_gap  = 0,
    };
    const ink_ui_size_t text_sz = ink_ui_text_measure(&s_canvas, title, bounds.width, &style);
    const ink_ui_rect_t text_r  = ink_ui_rect_align(bar, text_sz, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_text_draw(&s_canvas, text_r, title, &style, INK_UI_PAINT_WHITE);

    const ink_ui_rect_t body = {
        .x      = 0,
        .y      = title_height,
        .width  = bounds.width,
        .height = (int16_t)(bounds.height - title_height),
    };
    return ink_ui_rect_inset(body, 8, 8);
}

/* Screen 1, row 1: four outlined boxes side by side: 1 px, 2 px, 4 px square corners, and 3 px with radius 8. */
static void draw_stroke_samples(ink_ui_rect_t area) {
    const ink_ui_length_t lengths[4] = {{.flex = 1}, {.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         boxes[4];
    ink_ui_rect_split(area, INK_UI_AXIS_ROW, 6, lengths, 4, boxes);
    ink_ui_stroke(&s_canvas, boxes[0], 1, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(&s_canvas, boxes[1], 2, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(&s_canvas, boxes[2], 4, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(&s_canvas, boxes[3], 3, 8, INK_UI_PAINT_BLACK);
}

/* Screen 1, row 2: five filled boxes side by side, brightness 0, 64, 128, 192, 255, each with a 1 px black outline. */
static void draw_dither_ramp(ink_ui_rect_t area) {
    const ink_ui_length_t lengths[5] = {{.flex = 1}, {.flex = 1}, {.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         boxes[5];
    static const uint8_t  brightnesses[5] = {0, 64, 128, 192, 255};
    ink_ui_rect_split(area, INK_UI_AXIS_ROW, 6, lengths, 5, boxes);
    for (int i = 0; i < 5; i++) {
        ink_ui_fill(&s_canvas, boxes[i], 0, INK_UI_PAINT_GRAY(brightnesses[i]));
        ink_ui_stroke(&s_canvas, boxes[i], 1, 0, INK_UI_PAINT_BLACK);
    }
}

/* Screen 1, row 3: both diagonals of `area`, 1 px and 3 px, plus one horizontal 2 px line through the middle. */
static void draw_line_samples(ink_ui_rect_t area) {
    ink_ui_line(&s_canvas, area.x, area.y, (int16_t)(area.x + area.width - 1), (int16_t)(area.y + area.height - 1), 1, INK_UI_PAINT_BLACK);
    ink_ui_line(&s_canvas, (int16_t)(area.x + area.width - 1), area.y, area.x, (int16_t)(area.y + area.height - 1), 3, INK_UI_PAINT_BLACK);
    const int16_t mid_y = (int16_t)(area.y + area.height / 2);
    ink_ui_line(&s_canvas, area.x, mid_y, (int16_t)(area.x + area.width - 1), mid_y, 2, INK_UI_PAINT_BLACK);
}

/* Screen 1, row 4: one filled box with radius 10, and a 16 × 16 MASK1 bitmap (an arrow pointing right) drawn once
 * in black on white and once in white on a black box. */
static void draw_bitmap_sample(ink_ui_rect_t area) {
    const ink_ui_length_t lengths[3] = {{.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         cols[3];
    ink_ui_rect_split(area, INK_UI_AXIS_ROW, 8, lengths, 3, cols);

    ink_ui_fill(&s_canvas, cols[0], 10, INK_UI_PAINT_GRAY(128));

    const ink_ui_rect_t bm1 = ink_ui_rect_align(cols[1], (ink_ui_size_t){16, 16}, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_bitmap(&s_canvas, bm1.x, bm1.y, &s_arrow_bitmap, INK_UI_PAINT_BLACK);

    ink_ui_fill(&s_canvas, cols[2], 0, INK_UI_PAINT_BLACK);
    const ink_ui_rect_t bm2 = ink_ui_rect_align(cols[2], (ink_ui_size_t){16, 16}, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_bitmap(&s_canvas, bm2.x, bm2.y, &s_arrow_bitmap, INK_UI_PAINT_WHITE);
}

/* Screen 2, row 1: three one-line texts, aligned START, CENTER and END, each under the other. */
static void draw_aligned_lines(ink_ui_rect_t area) {
    const ink_ui_length_t lengths[3] = {{.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         rows[3];
    ink_ui_rect_split(area, INK_UI_AXIS_COLUMN, 2, lengths, 3, rows);

    const ink_ui_text_style_t s0 = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_START, .max_lines = 1, .line_gap = 0};
    ink_ui_text_draw(&s_canvas, rows[0], "Căn trái: bắt đầu", &s0, INK_UI_PAINT_BLACK);

    const ink_ui_text_style_t s1 = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_CENTER, .max_lines = 1, .line_gap = 0};
    ink_ui_text_draw(&s_canvas, rows[1], "Căn giữa: cân bằng", &s1, INK_UI_PAINT_BLACK);

    const ink_ui_text_style_t s2 = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_END, .max_lines = 1, .line_gap = 0};
    ink_ui_text_draw(&s_canvas, rows[2], "Căn phải: kết thúc", &s2, INK_UI_PAINT_BLACK);
}

/* Screen 2, row 2: a 1 px outline around `area` and a paragraph of about 150 characters that wraps inside it. */
static void draw_wrapped_paragraph(ink_ui_rect_t area) {
    ink_ui_stroke(&s_canvas, area, 1, 0, INK_UI_PAINT_BLACK);
    const ink_ui_rect_t       inner = ink_ui_rect_inset(area, 4, 4);
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 0,
        .line_gap  = 0,
    };
    ink_ui_text_draw(&s_canvas, inner, s_sample_paragraph, &style, INK_UI_PAINT_BLACK);
}

/* Screen 2, row 3: a 1 px outline around `area` and the same paragraph limited to max_lines = 2, so it ends with
 * an ellipsis. */
static void draw_truncated_paragraph(ink_ui_rect_t area) {
    ink_ui_stroke(&s_canvas, area, 1, 0, INK_UI_PAINT_BLACK);
    const ink_ui_rect_t       inner = ink_ui_rect_inset(area, 4, 4);
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 2,
        .line_gap  = 0,
    };
    ink_ui_text_draw(&s_canvas, inner, s_sample_paragraph, &style, INK_UI_PAINT_BLACK);
}

/* Screen 3: split `area` into GRID_CELLS cells (2 columns × 3 rows, 8 px gaps) with ink_ui_rect_split(). */
static void split_grid(ink_ui_rect_t area, ink_ui_rect_t out_cells[GRID_CELLS]) {
    const ink_ui_length_t row_lengths[3] = {{.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         rows[3];
    ink_ui_rect_split(area, INK_UI_AXIS_COLUMN, 8, row_lengths, 3, rows);
    const ink_ui_length_t col_lengths[2] = {{.flex = 1}, {.flex = 1}};
    for (int r = 0; r < 3; r++) {
        ink_ui_rect_t cols[2];
        ink_ui_rect_split(rows[r], INK_UI_AXIS_ROW, 8, col_lengths, 2, cols);
        out_cells[r * 2 + 0] = cols[0];
        out_cells[r * 2 + 1] = cols[1];
    }
}

/* Screen 3: draw every cell as a 2 px outlined box with its number (1 .. GRID_CELLS) centered inside. */
static void draw_grid(const ink_ui_rect_t cells[GRID_CELLS]) {
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_CENTER,
        .max_lines = 1,
        .line_gap  = 0,
    };
    for (uint8_t i = 0; i < GRID_CELLS; i++) {
        ink_ui_stroke(&s_canvas, cells[i], 2, 0, INK_UI_PAINT_BLACK);
        char label[2];
        label[0]               = (char)('1' + i);
        label[1]               = '\0';
        const ink_ui_size_t sz = ink_ui_text_measure(&s_canvas, label, cells[i].width, &style);
        const ink_ui_rect_t tr = ink_ui_rect_align(cells[i], sz, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
        ink_ui_text_draw(&s_canvas, tr, label, &style, INK_UI_PAINT_BLACK);
    }
}

/* Screen 3: invert `cell` with an XOR fill. */
static void invert_cell(ink_ui_rect_t cell) {
    ink_ui_fill(&s_canvas, cell, 0, INK_UI_PAINT_XOR);
}

/* Screen 4, frame 1: draw an empty card (3 px outline, radius 12) centered in `area` and return the inside of the
 * card, inset by 12 px. */
static ink_ui_rect_t draw_card_outline(ink_ui_rect_t area) {
    const ink_ui_size_t card_size = {
        .width  = (int16_t)(area.width > 24 ? area.width - 16 : area.width),
        .height = (int16_t)(area.height > 24 ? area.height - 16 : area.height),
    };
    const ink_ui_rect_t card = ink_ui_rect_align(area, card_size, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_stroke(&s_canvas, card, 3, 12, INK_UI_PAINT_BLACK);
    return ink_ui_rect_inset(card, 12, 12);
}

/* Screen 4, frame 2: draw a Vietnamese question of 2 to 3 lines, centered, inside `inside`. */
static void draw_card_text(ink_ui_rect_t inside) {
    const char               *question = "Thủ đô của Việt Nam\nlà thành phố nào?";
    const ink_ui_text_style_t style    = {
           .font      = &ink_ui_font_unifont_vi_16,
           .align     = INK_UI_ALIGN_CENTER,
           .max_lines = 3,
           .line_gap  = 4,
    };
    const ink_ui_size_t sz = ink_ui_text_measure(&s_canvas, question, inside.width, &style);
    const ink_ui_rect_t tr = ink_ui_rect_align(inside, sz, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_text_draw(&s_canvas, tr, question, &style, INK_UI_PAINT_BLACK);
}

/* ---- One frame ---- */

/* Acquire a frame and bind the canvas to it; the frame holds the latest submitted image. */
static bool begin_frame(void) {
    uint8_t *frame = ink_display_acquire();
    if (frame == nullptr) {
        ESP_LOGE(TAG, "acquire");
        return false;
    }
    ink_ui_canvas_bind(&s_canvas, frame);
    return true;
}

static void end_frame(ink_display_intent_t intent, uint8_t flags) {
    uint8_t        *frame = ink_ui_canvas_frame(&s_canvas, nullptr);
    const esp_err_t error = ink_display_submit(frame, ink_platform_now_ms(), intent, flags);
    if (error != ESP_OK) { ESP_LOGE(TAG, "submit: %s", esp_err_to_name(error)); }
    ink_ui_canvas_bind(&s_canvas, nullptr);
}

/* ---- Screens ---- */

static void screen_primitives(void) {
    if (!begin_frame()) { return; }
    const ink_ui_rect_t   body      = draw_page("1/4 Primitive, paint");
    const ink_ui_length_t lengths[] = {{.flex = 1}, {.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         rows[4];
    ink_ui_rect_split(body, INK_UI_AXIS_COLUMN, 8, lengths, 4, rows);
    draw_stroke_samples(rows[0]);
    draw_dither_ramp(rows[1]);
    draw_line_samples(rows[2]);
    draw_bitmap_sample(rows[3]);
    end_frame(INK_DISPLAY_INTENT_PAGE, 0);
}

static void screen_text(void) {
    if (!begin_frame()) { return; }
    const ink_ui_rect_t   body      = draw_page("2/4 Chữ tiếng Việt");
    const ink_ui_length_t lengths[] = {{.fixed = 64}, {.flex = 1}, {.fixed = 56}};
    ink_ui_rect_t         rows[3];
    ink_ui_rect_split(body, INK_UI_AXIS_COLUMN, 8, lengths, 3, rows);
    draw_aligned_lines(rows[0]);
    draw_wrapped_paragraph(rows[1]);
    draw_truncated_paragraph(rows[2]);
    end_frame(INK_DISPLAY_INTENT_PAGE, 0);
}

static void screen_rulers(void) {
    if (!begin_frame()) { return; }
    const ink_ui_rect_t body = draw_page("3/4 Thước, XOR");
    ink_ui_rect_t       cells[GRID_CELLS];
    split_grid(body, cells);
    draw_grid(cells);
    invert_cell(cells[s_selected]);
    s_selected = (uint8_t)((s_selected + 1) % GRID_CELLS);
    end_frame(INK_DISPLAY_INTENT_PAGE, 0);
}

/* Two frames in a row, as an app that shows the card first and its text second. */
static void screen_two_phase(void) {
    if (!begin_frame()) { return; }
    const ink_ui_rect_t body   = draw_page("4/4 Hai nhịp");
    const ink_ui_rect_t inside = draw_card_outline(body);
    end_frame(INK_DISPLAY_INTENT_UI, INK_DISPLAY_FLAG_DEFER_CLEANUP);

    /* HACK(demo): the display report does not reach the main task yet, so nothing tells when the display task has
     * taken the first frame; submitting the second one at once would drop the first. Should wait for the report. */
    vTaskDelay(pdMS_TO_TICKS(PHASE_GAP_MS));

    if (!begin_frame()) { return; }
    draw_card_text(inside);
    end_frame(INK_DISPLAY_INTENT_TEXT, 0);
}

static void show_screen(void) {
    ESP_LOGI(TAG, "screen %d/%d", s_screen + 1, SCREEN_COUNT);
    switch (s_screen) {
        case 0:
            screen_primitives();
            break;
        case 1:
            screen_text();
            break;
        case 2:
            screen_rulers();
            break;
        default:
            screen_two_phase();
            break;
    }
}

/* ---- API ---- */

esp_err_t ink_demo_start(void) {
    ink_display_frame_info_t info;
    ESP_RETURN_ON_ERROR(ink_display_frame_info(&info), TAG, "frame info");
    ESP_RETURN_ON_FALSE(ink_ui_canvas_init(&s_canvas, &info, INK_UI_ROTATION_0) == INK_UI_OK, ESP_ERR_NOT_SUPPORTED, TAG, "canvas");
    s_started = true;
    show_screen();
    return ESP_OK;
}

void ink_demo_handle(const ink_gesture_output_t *gestures) {
    if (!s_started || gestures == nullptr) { return; }
    for (uint8_t i = 0; i < gestures->count; i++) {
        const int8_t step = screen_step(&gestures->items[i]);
        if (step == 0) { continue; }
        s_screen = (uint8_t)((s_screen + SCREEN_COUNT + step) % SCREEN_COUNT);
        show_screen();
    }
}
