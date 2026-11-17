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

/* NOTE: used by draw_cursor() once its body exists; maybe_unused until then. */
[[maybe_unused]] static const ink_ui_bitmap_t s_arrow = {
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

/* Text of an item: the name of app (index + 1), or "Ngủ" for the last item. */
[[maybe_unused]] static const char *item_label(uint8_t index) {
    /* LEAF: last item (index == item_count() - 1) -> "Ngủ"; otherwise ink_system_app_at(index + 1)->name; "" if NULL. */
    (void)index;
    return "";
}

/* Cursor position after `step` (+1 or -1) from `index` among `count` items, wrapping at both ends. */
static uint8_t cursor_step(uint8_t index, uint8_t count, int8_t step) {
    /* LEAF: (index + step) modulo count without going negative; return index when count is 0. */
    (void)count;
    (void)step;
    return index;
}

/* Rectangle of item row `index` over the full canvas width: y = ROW_TOP + index * ROW_HEIGHT, height ROW_HEIGHT. */
[[maybe_unused]] static ink_ui_rect_t item_row_rect(uint8_t index) {
    /* LEAF: build the rect from the layout constants and ink_ui_canvas_bounds(&s_canvas).width. */
    (void)index;
    return (ink_ui_rect_t){0};
}

/* Clear the canvas to white and draw the black title bar with "Home" centered in white (Unifont 16 px). */
static void draw_background(void) {
    /* LEAF: ink_ui_fill white over the bounds, black bar of TITLE_HEIGHT, centered white text with ink_ui_text_*. */
}

/* Draw the label of every item left-aligned at LABEL_X, vertically centered in its row, black Unifont 16 px. */
static void draw_items(void) {
    /* LEAF: for each index below item_count(): item_row_rect(index), inset to start at LABEL_X, ink_ui_text_draw. */
}

/* Draw the arrow of item `index` at ARROW_X, vertically centered in its row (visible), or clear its 16 × 16 box to
 * white (not visible). Touches nothing outside that box. */
static void draw_cursor(uint8_t index, bool visible) {
    /* LEAF: ink_ui_bitmap(&s_arrow, black) when visible; ink_ui_fill white over the arrow box otherwise. */
    (void)index;
    (void)visible;
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
