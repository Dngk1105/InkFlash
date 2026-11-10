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
/* LEAF: every prototype in this block gets its definition here, in place of the prototype. All of them draw on
 * s_canvas with ink_ui only; none acquires or submits. Text is Vietnamese with diacritics, font
 * ink_ui_font_unifont_vi_16. */

/* +1 for PRESS of button A without modifier, -1 for PRESS of button B without modifier, 0 for anything else. */
static int8_t screen_step(const ink_gesture_t *gesture);

/* Clear the canvas to white, draw a title bar (black bar, white text `title`) at the top and return the area below
 * it, inset by 8 px. */
static ink_ui_rect_t draw_page(const char *title);

/* Screen 1, row 1: four outlined boxes side by side: 1 px, 2 px, 4 px square corners, and 3 px with radius 8. */
static void draw_stroke_samples(ink_ui_rect_t area);

/* Screen 1, row 2: five filled boxes side by side, brightness 0, 64, 128, 192, 255, each with a 1 px black outline. */
static void draw_dither_ramp(ink_ui_rect_t area);

/* Screen 1, row 3: both diagonals of `area`, 1 px and 3 px, plus one horizontal 2 px line through the middle. */
static void draw_line_samples(ink_ui_rect_t area);

/* Screen 1, row 4: one filled box with radius 10, and a 16 × 16 MASK1 bitmap (an arrow pointing right) drawn once
 * in black on white and once in white on a black box. */
static void draw_bitmap_sample(ink_ui_rect_t area);

/* Screen 2, row 1: three one-line texts, aligned START, CENTER and END, each under the other. */
static void draw_aligned_lines(ink_ui_rect_t area);

/* Screen 2, row 2: a 1 px outline around `area` and a paragraph of about 150 characters that wraps inside it. */
static void draw_wrapped_paragraph(ink_ui_rect_t area);

/* Screen 2, row 3: a 1 px outline around `area` and the same paragraph limited to max_lines = 2, so it ends with
 * an ellipsis. */
static void draw_truncated_paragraph(ink_ui_rect_t area);

/* Screen 3: split `area` into GRID_CELLS cells (2 columns × 3 rows, 8 px gaps) with ink_ui_rect_split(). */
static void split_grid(ink_ui_rect_t area, ink_ui_rect_t out_cells[GRID_CELLS]);

/* Screen 3: draw every cell as a 2 px outlined box with its number (1 .. GRID_CELLS) centered inside. */
static void draw_grid(const ink_ui_rect_t cells[GRID_CELLS]);

/* Screen 3: invert `cell` with an XOR fill. */
static void invert_cell(ink_ui_rect_t cell);

/* Screen 4, frame 1: draw an empty card (3 px outline, radius 12) centered in `area` and return the inside of the
 * card, inset by 12 px. */
static ink_ui_rect_t draw_card_outline(ink_ui_rect_t area);

/* Screen 4, frame 2: draw a Vietnamese question of 2 to 3 lines, centered, inside `inside`. */
static void draw_card_text(ink_ui_rect_t inside);

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
