/* Design: docs/design/figma-to-code.md */
#include "ink_app_foundations.h"

#include "esp_log.h"
#include "ink_display_frame.h"
#include "ink_platform.h"
#include "ink_ui.h"
#include "views/foundations_landscape_view.gen.h"
#include "views/foundations_portrait_view.gen.h"

static const char *TAG = "ink_app_foundations";

/* ---- Scenes ---- */

enum : uint8_t {
    SCENE_MAIN = 0, /* root scene; state byte 0 = page shown (PAGE_*) */
    SCENE_COUNT,
};

enum : uint8_t {
    PAGE_PORTRAIT = 0, /* Figma frame foundations_portrait, 240 x 416 */
    PAGE_LANDSCAPE,    /* Figma frame foundations_landscape, 416 x 240 on a canvas turned 90° */
    PAGE_COUNT,
};

/* ---- Shared resources (created in start) ---- */

static ink_ui_canvas_t s_portrait;
static ink_ui_canvas_t s_landscape;

/* ---- One frame ---- */

/* Acquire a frame and bind `canvas` to it; the frame already holds the latest submitted image. */
static bool begin_frame(ink_ui_canvas_t *canvas) {
    uint8_t *frame = ink_display_acquire();
    if (frame == nullptr) {
        ESP_LOGE(TAG, "acquire");
        return false;
    }
    ink_ui_canvas_bind(canvas, frame);
    return true;
}

/* Submit the drawn frame; the canvas is unbound so nothing draws into a submitted frame. */
static void end_frame(ink_ui_canvas_t *canvas, ink_display_intent_t intent, uint8_t flags) {
    uint8_t        *frame = ink_ui_canvas_frame(canvas, nullptr);
    const esp_err_t error = ink_display_submit(frame, ink_platform_now_ms(), intent, flags);
    if (error != ESP_OK) { ESP_LOGE(TAG, "submit: %s", esp_err_to_name(error)); }
    ink_ui_canvas_bind(canvas, nullptr);
}

/* ---- Scene main ---- */

/* Draw page `page` over the whole screen: intent PAGE, every pixel may change. */
static void draw_page(uint8_t page) {
    if (page == PAGE_LANDSCAPE) {
        if (!begin_frame(&s_landscape)) { return; }
        const foundations_landscape_view_data_t data = {0};
        foundations_landscape_view_draw(&s_landscape, &data);
        end_frame(&s_landscape, INK_DISPLAY_INTENT_PAGE, 0);
        return;
    }
    if (!begin_frame(&s_portrait)) { return; }
    const foundations_portrait_view_data_t data = {0};
    foundations_portrait_view_draw(&s_portrait, &data);
    end_frame(&s_portrait, INK_DISPLAY_INTENT_PAGE, 0);
}

static void main_enter(uint8_t *state, bool restored) {
    if (!restored || state[0] >= PAGE_COUNT) { state[0] = PAGE_PORTRAIT; }
}

static void main_draw(const uint8_t *state) {
    draw_page(state[0]);
}

/* NEXT and SELECT switch between the portrait and the landscape page. */
static bool main_command(uint8_t *state, ink_keymap_command_t command) {
    switch (command) {
        case INK_KEYMAP_COMMAND_NEXT:
        case INK_KEYMAP_COMMAND_SELECT:
            state[0] = (uint8_t)((state[0] + 1) % PAGE_COUNT);
            draw_page(state[0]);
            return true;
        default:
            return false; /* BACK goes back to system (Home) */
    }
}

static const ink_system_scene_t s_scenes[SCENE_COUNT] = {
    [SCENE_MAIN] = {.enter = main_enter, .command = main_command, .draw = main_draw, .keymap = nullptr},
};

/* ---- App ---- */

/* A PRESS next page, B PRESS next page, B HOLD back to Home. */
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

/* Create one canvas per orientation; runs on the main task, after the display started. */
static esp_err_t app_start(const uint8_t *snapshot, size_t length) {
    (void)snapshot; /* no app-level state */
    (void)length;
    ink_display_frame_info_t info;
    const esp_err_t          error = ink_display_frame_info(&info);
    if (error != ESP_OK) { return error; }
    if (ink_ui_canvas_init(&s_portrait, &info, INK_UI_ROTATION_0) != INK_UI_OK) { return ESP_ERR_NOT_SUPPORTED; }
    if (ink_ui_canvas_init(&s_landscape, &info, INK_UI_ROTATION_90) != INK_UI_OK) { return ESP_ERR_NOT_SUPPORTED; }
    return ESP_OK;
}

const ink_system_app_t ink_app_foundations = {
    .name        = "Foundations",
    .start       = app_start,
    .save        = nullptr,
    .stop        = nullptr,
    .app_table   = &s_app_table,
    .scenes      = s_scenes,
    .scene_count = SCENE_COUNT,
};
