/* Design: docs/design/app-framework.md */
#include "ink_app_home.h"

#include "esp_log.h"
#include "ink_display_frame.h"
#include "ink_platform.h"
#include "ink_ui.h"

static const char *TAG = "ink_app_home";

/* ---- Scenes ---- */

enum : uint8_t {
    HOME_SCENE_LIST = 0, /* root scene; state byte 0 = selected item */
    HOME_SCENE_COUNT,
};

/* ---- Layout ---- */
/* Left and right edges of everything that changes on a cursor move sit on the 8 px grid (app-ui-guide.md). */

#define TITLE_HEIGHT 24 /* black title bar, px */
#define ROW_TOP      32 /* y of the first item row, px */
#define ROW_HEIGHT   32 /* height of one item row, px */
#define ARROW_X      8  /* left edge of the cursor arrow, px */
#define ARROW_SIZE   16 /* arrow bitmap width and height, px */
#define LABEL_X      32 /* left edge of an item label, px */

/* 16 × 16 MASK1 arrow pointing right. */
static const uint8_t s_arrow_data[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x30, 0x00, 0x38, 0x00, 0x3C, 0xFF, 0xFE, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x3C, 0x00, 0x38, 0x00, 0x30, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
};

static const ink_ui_bitmap_t s_arrow = {
    .width  = ARROW_SIZE,
    .height = ARROW_SIZE,
    .format = INK_UI_BITMAP_FORMAT_MASK1,
    .data   = s_arrow_data,
};

/* ---- Shared resources (created in start) ---- */

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

/* Submit the drawn frame; the canvas is unbound so nothing draws into a submitted frame. */
static void end_frame(ink_display_intent_t intent, uint8_t flags) {
    uint8_t        *frame = ink_ui_canvas_frame(&s_canvas, nullptr);
    const esp_err_t error = ink_display_submit(frame, ink_platform_now_ms(), intent, flags);
    if (error != ESP_OK) { ESP_LOGE(TAG, "submit: %s", esp_err_to_name(error)); }
    ink_ui_canvas_bind(&s_canvas, nullptr);
}

/* ---- Leaf functions ---- */

/* Number of list items: every app except Home, plus the sleep item at the end. */
static uint8_t item_count(void) {
    return ink_system_app_count();
}

/* Text of an item: the name of app (index + 1), or the sleep item for the last item. */
static const char *item_label(uint8_t index) {
    const uint8_t count = item_count();
    if (count == 0 || index >= count) { return ""; }
    if (index == count - 1) { return "Ngủ"; }
    const ink_system_app_t *app = ink_system_app_at(index + 1);
    if (app == nullptr || app->name == nullptr) { return ""; }
    return app->name;
}

/* Cursor position after `step` (+1 or -1) from `index` among `count` items, wrapping at both ends. */
static uint8_t cursor_step(uint8_t index, uint8_t count, int8_t step) {
    if (count == 0) { return index; }
    int16_t next = (int16_t)index + step;
    next         = next % (int16_t)count;
    if (next < 0) { next += count; }
    return (uint8_t)next;
}

/* Rectangle of item row `index` over the full canvas width: y = ROW_TOP + index * ROW_HEIGHT, height ROW_HEIGHT. */
static ink_ui_rect_t item_row_rect(uint8_t index) {
    return (ink_ui_rect_t){
        .x      = 0,
        .y      = (int16_t)(ROW_TOP + index * ROW_HEIGHT),
        .width  = ink_ui_canvas_bounds(&s_canvas).width,
        .height = ROW_HEIGHT,
    };
}

/* Clear the canvas to white and draw the black title bar with "Home" centered in white (Unifont 16 px). */
static void draw_background(void) {
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(&s_canvas);
    ink_ui_fill(&s_canvas, bounds, 0, INK_UI_PAINT_WHITE);
    const ink_ui_rect_t bar = {
        .x      = 0,
        .y      = 0,
        .width  = bounds.width,
        .height = TITLE_HEIGHT,
    };
    ink_ui_fill(&s_canvas, bar, 0, INK_UI_PAINT_BLACK);
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_CENTER,
        .max_lines = 1,
    };
    const ink_ui_size_t size = ink_ui_text_measure(&s_canvas, "Home", bar.width, &style);
    const ink_ui_rect_t area = ink_ui_rect_align(bar, size, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_text_draw(&s_canvas, area, "Home", &style, INK_UI_PAINT_WHITE);
}

/* Draw the label of every item left-aligned at LABEL_X, vertically centered in its row, black Unifont 16 px. */
static void draw_items(void) {
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 1,
    };
    const uint8_t count = item_count();
    for (uint8_t i = 0; i < count; i++) {
        const ink_ui_rect_t row        = item_row_rect(i);
        const ink_ui_rect_t label_rect = {
            .x      = LABEL_X,
            .y      = row.y,
            .width  = (int16_t)(row.width - LABEL_X),
            .height = row.height,
        };
        const char         *label = item_label(i);
        const ink_ui_size_t size  = ink_ui_text_measure(&s_canvas, label, label_rect.width, &style);
        const ink_ui_rect_t area  = ink_ui_rect_align(label_rect, size, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER);
        ink_ui_text_draw(&s_canvas, area, label, &style, INK_UI_PAINT_BLACK);
    }
}

/* Draw the arrow of item `index` at ARROW_X, vertically centered in its row (visible), or clear its 16 × 16 box to
 * white (not visible). Touches nothing outside that box. */
static void draw_cursor(uint8_t index, bool visible) {
    const ink_ui_rect_t row = item_row_rect(index);
    const ink_ui_rect_t box = {
        .x      = ARROW_X,
        .y      = (int16_t)(row.y + (ROW_HEIGHT - ARROW_SIZE) / 2),
        .width  = ARROW_SIZE,
        .height = ARROW_SIZE,
    };
    if (visible) {
        ink_ui_bitmap(&s_canvas, box.x, box.y, &s_arrow, INK_UI_PAINT_BLACK);
    } else {
        ink_ui_fill(&s_canvas, box, 0, INK_UI_PAINT_WHITE);
    }
}

/* ---- Item actions ---- */

/* Open the app of item `index`; the sleep item does nothing yet. */
static void activate_item(uint8_t index) {
    if (index + 1 >= item_count()) {
        /* TODO(home): sleep item: ask system to power off once the session block exists. */
        ESP_LOGW(TAG, "sleep item: session not built");
        return;
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(ink_system_open_app(index + 1));
}

/* ---- Scene list ---- */

static void list_enter(uint8_t *state, bool restored) {
    if (!restored) { state[0] = 0; }
}

/* Whole screen: intent PAGE, because the scene just opened or came back (BACK). */
static void list_draw(const uint8_t *state) {
    if (!begin_frame()) { return; }
    draw_background();
    draw_items();
    draw_cursor(state[0], true);
    end_frame(INK_DISPLAY_INTENT_PAGE, 0);
}

/* Move the cursor and redraw only the two arrows: intent UI, a small change. */
static void list_move_cursor(uint8_t *state, int8_t step) {
    const uint8_t previous = state[0];
    state[0]               = cursor_step(previous, item_count(), step);
    if (state[0] == previous || !begin_frame()) { return; }
    draw_cursor(previous, false);
    draw_cursor(state[0], true);
    end_frame(INK_DISPLAY_INTENT_UI, 0);
}

static bool list_command(uint8_t *state, ink_keymap_command_t command) {
    switch (command) {
        case INK_KEYMAP_COMMAND_NEXT:
            list_move_cursor(state, 1);
            return true;
        case INK_KEYMAP_COMMAND_PREV:
            list_move_cursor(state, -1);
            return true;
        case INK_KEYMAP_COMMAND_SELECT:
            activate_item(state[0]);
            return true;
        default:
            return false; /* BACK at the root of Home is ignored by system */
    }
}

static const ink_system_scene_t s_scenes[HOME_SCENE_COUNT] = {
    [HOME_SCENE_LIST] = {.enter = list_enter, .command = list_command, .draw = list_draw, .keymap = nullptr},
};

/* ---- App ---- */

/* A PRESS next, A DOUBLE_PRESS previous, B PRESS select, B HOLD back (ignored at the root of Home, but the table must reach BACK). */
static const ink_keymap_table_t s_app_table = {
    .count = 4,
    .entries =
        {
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_A, INK_GESTURE_TYPE_PRESS},
             .command = INK_KEYMAP_COMMAND_NEXT},
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_A, INK_GESTURE_TYPE_DOUBLE_PRESS},
             .command = INK_KEYMAP_COMMAND_PREV},
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_PRESS},
             .command = INK_KEYMAP_COMMAND_SELECT},
                  {.gesture = {INK_GESTURE_MODIFIER_NONE, INK_GESTURE_SOURCE_BUTTON, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_HOLD},
             .command = INK_KEYMAP_COMMAND_BACK},
                  },
};

/* Create the canvas the scene draws with; runs on the main task, after the display started. */
static esp_err_t home_start(const uint8_t *snapshot, size_t length) {
    (void)snapshot; /* no app-level state */
    (void)length;
    ink_display_frame_info_t info;
    const esp_err_t          error = ink_display_frame_info(&info);
    if (error != ESP_OK) { return error; }
    if (ink_ui_canvas_init(&s_canvas, &info, INK_UI_ROTATION_0) != INK_UI_OK) { return ESP_ERR_NOT_SUPPORTED; }
    return ESP_OK;
}

const ink_system_app_t ink_app_home = {
    .name        = "Home",
    .start       = home_start,
    .save        = nullptr,
    .stop        = nullptr,
    .app_table   = &s_app_table,
    .scenes      = s_scenes,
    .scene_count = HOME_SCENE_COUNT,
};
