/* Design: docs/design/app-framework.md */
#include "ink_app_example.h"

#include "esp_log.h"
#include "ink_display_frame.h"
#include "ink_platform.h"
#include "ink_ui.h"
#include <stdio.h>

static const char *TAG = "ink_app_example";

/* ---- Scenes ---- */

enum : uint8_t {
    SCENE_MAIN = 0, /* root scene; state byte 0 = counter */
    SCENE_COUNT,
};

/* ---- Shared resources (created in start, shared by every scene) ---- */

static ink_ui_canvas_t s_canvas;

/* ---- One frame ---- */

/* Acquire a frame and bind the canvas to it; the frame already holds the latest submitted image. */
static bool begin_frame(void) {
    uint8_t *frame = ink_display_acquire();
    if (frame == nullptr) {
        ESP_LOGE(TAG, "acquire");
        return false;
    }
    ink_ui_canvas_bind(&s_canvas, frame);
    return true;
}

/* Submit the drawn frame; the canvas is unbound so no scene draws into a submitted frame. */
static void end_frame(ink_display_intent_t intent, uint8_t flags) {
    uint8_t        *frame = ink_ui_canvas_frame(&s_canvas, nullptr);
    const esp_err_t error = ink_display_submit(frame, ink_platform_now_ms(), intent, flags);
    if (error != ESP_OK) { ESP_LOGE(TAG, "submit: %s", esp_err_to_name(error)); }
    ink_ui_canvas_bind(&s_canvas, nullptr);
}

/* ---- Scene main ---- */

/* Draw the counter centered; the area is cleared first so a shorter number leaves nothing behind. */
static void draw_counter(uint8_t counter) {
    const ink_ui_rect_t       bounds = ink_ui_canvas_bounds(&s_canvas);
    const ink_ui_text_style_t style  = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_CENTER, .max_lines = 1};
    char                      text[8];
    snprintf(text, sizeof(text), "%u", (unsigned)counter);
    ink_ui_fill(&s_canvas, bounds, 0, INK_UI_PAINT_WHITE);
    const ink_ui_size_t size = ink_ui_text_measure(&s_canvas, text, bounds.width, &style);
    const ink_ui_rect_t area = ink_ui_rect_align(bounds, size, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_text_draw(&s_canvas, area, text, &style, INK_UI_PAINT_BLACK);
}

static void main_enter(uint8_t *state, bool restored) {
    if (!restored) { state[0] = 0; }
}

static void main_draw(const uint8_t *state) {
    if (!begin_frame()) { return; }
    draw_counter(state[0]);
    end_frame(INK_DISPLAY_INTENT_PAGE, 0);
}

static bool main_command(uint8_t *state, ink_keymap_command_t command) {
    switch (command) {
        case INK_KEYMAP_COMMAND_NEXT:
            state[0]++;
            break;
        case INK_KEYMAP_COMMAND_SELECT:
            state[0] = 0;
            break;
        default:
            return false; /* BACK and unknown commands go back to system */
    }
    /* A change inside the scene draws at once (intent UI for a small change), without waiting for draw(). */
    if (begin_frame()) {
        draw_counter(state[0]);
        end_frame(INK_DISPLAY_INTENT_UI, 0);
    }
    return true;
}

static const ink_system_scene_t s_scenes[SCENE_COUNT] = {
    [SCENE_MAIN] = {.enter = main_enter, .command = main_command, .draw = main_draw, .keymap = nullptr},
};

/* ---- App ---- */

static const ink_keymap_table_t s_app_table = {
    .count = 3,
    .entries =
        {
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_A, INK_GESTURE_TYPE_PRESS},
             .command = INK_KEYMAP_COMMAND_NEXT},
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_PRESS},
             .command = INK_KEYMAP_COMMAND_SELECT},
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_HOLD},
             .command = INK_KEYMAP_COMMAND_BACK},
                  },
};

/* Create what every scene shares: here only the canvas. start() runs on the main task, after the display started. */
static esp_err_t app_start(const uint8_t *snapshot, size_t length) {
    (void)snapshot; /* no app-level state: the scene stack is all there is to restore */
    (void)length;
    ink_display_frame_info_t info;
    const esp_err_t          error = ink_display_frame_info(&info);
    if (error != ESP_OK) { return error; }
    if (ink_ui_canvas_init(&s_canvas, &info, INK_UI_ROTATION_0) != INK_UI_OK) { return ESP_ERR_NOT_SUPPORTED; }
    return ESP_OK;
}

const ink_system_app_t ink_app_example = {
    .name        = "Example",
    .start       = app_start,
    .save        = nullptr,
    .stop        = nullptr,
    .app_table   = &s_app_table,
    .scenes      = s_scenes,
    .scene_count = SCENE_COUNT,
};
