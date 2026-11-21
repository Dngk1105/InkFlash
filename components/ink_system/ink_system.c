/* Design: docs/design/tasks.md, docs/design/wake-session.md */
#include "ink_system.h"

#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_display.h"
#include "ink_input.h"
#include "ink_keymap.h"
#include "ink_platform.h"
#include "ink_scene.h"
#include "ink_session.h"
#include "ink_ui.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "ink_system";

/* ---- Messages to the main task ---- */
/* ISRs and timers of every module only build messages; all work happens on the main task. */

typedef enum : uint8_t {
    MESSAGE_SOURCE_INPUT = 0,
    MESSAGE_SOURCE_DISPLAY, /* report of the display task: a frame was drawn, a cleanup ran... */
    MESSAGE_SOURCE_BOOT,    /* boot finished: open the first app on the main task (apps draw only from the main task) */
    MESSAGE_SOURCE_IDLE,    /* the idle timer of the wake session fired */
    /* EXTEND(system): one source per module that posts messages (storage...). */
} message_source_t;

typedef struct {
    message_source_t source;

    union {
        ink_input_message_t  input;
        ink_display_report_t display; /* copy: the report of the display task is valid only during its callback */
    };
} message_t;

/* ---- Main queue ---- */

#define QUEUE_LENGTH 32 /* REVIEW(system): sized for one bounce on both buttons; not measured */

static QueueHandle_t s_queue;
static atomic_uint   s_dropped_count; /* messages dropped on a full queue; the ISR and helper tasks add, the main task reads */

/* Called from helper tasks (timer, display task); never waits. */
static void post_message(const message_t *message) {
    if (xQueueSend(s_queue, message, 0) != pdTRUE) { atomic_fetch_add(&s_dropped_count, 1); }
}

static void IRAM_ATTR post_message_from_isr(const message_t *message) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    if (xQueueSendFromISR(s_queue, message, &higher_priority_task_woken) != pdTRUE) { atomic_fetch_add(&s_dropped_count, 1); }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/* ---- Post functions given to each module ---- */

static void post_input(const ink_input_message_t *input) {
    const message_t message = {.source = MESSAGE_SOURCE_INPUT, .input = *input};
    post_message(&message);
}

static void IRAM_ATTR post_input_from_isr(const ink_input_message_t *input) {
    const message_t message = {.source = MESSAGE_SOURCE_INPUT, .input = *input};
    post_message_from_isr(&message);
}

/* Runs on the display task right after each piece of work; copies the report into a message and does not wait. */
static void post_display_report(const ink_display_report_t *report, void *user_data) {
    (void)user_data;
    const message_t message = {.source = MESSAGE_SOURCE_DISPLAY, .display = *report};
    post_message(&message);
}

/* ---- Open app and scene stack ---- */
/* All of this state is touched by the main task only. */

static const ink_system_app_t *const *s_apps;        /* fixed list from the config; s_apps[0] is Home */
static uint8_t                        s_app_count;   /* entries of s_apps */
static const ink_system_app_t        *s_app;         /* open app; nullptr until the boot message is handled */
static ink_scene_stack_t              s_stack;       /* scene stack of the open app */
static void (*s_after_report)(const uint8_t *state); /* one-shot continuation for the next display report; nullptr = none */

/* The scene on top of the stack and its state bytes. */
typedef struct {
    const ink_system_scene_t *scene;
    uint8_t                  *state;
} top_scene_t;

/* Fill `top` from the stack; false when no app is open or the stack is broken. */
static bool get_top_scene(top_scene_t *top) {
    if (s_app == nullptr) { return false; }
    ink_scene_entry_t *entry = ink_scene_top(&s_stack);
    if (entry == nullptr || entry->scene_id >= s_app->scene_count) { return false; }
    top->scene = &s_app->scenes[entry->scene_id];
    top->state = entry->state;
    return true;
}

static esp_err_t to_esp_err(ink_scene_error_t error) {
    switch (error) {
        case INK_SCENE_OK:
            return ESP_OK;
        case INK_SCENE_ERROR_FULL:
            return ESP_ERR_NO_MEM;
        case INK_SCENE_ERROR_EMPTY:
        case INK_SCENE_ERROR_SNAPSHOT:
            return ESP_ERR_INVALID_STATE;
        case INK_SCENE_ERROR_ARGUMENT:
            break;
    }
    return ESP_ERR_INVALID_ARG;
}

/* Tell input which gestures need waiting (double press, modifier) for the app table and the scene table on top. */
static void refresh_wait_masks(void) {
    top_scene_t top;
    if (!get_top_scene(&top)) { return; }
    const ink_gesture_button_t buttons[INK_GESTURE_BUTTON_COUNT] = {INK_GESTURE_BUTTON_A, INK_GESTURE_BUTTON_B};
    for (uint8_t i = 0; i < INK_GESTURE_BUTTON_COUNT; i++) {
        const ink_gesture_wait_mask_t mask = ink_keymap_wait_mask(top.scene->keymap, s_app->app_table, buttons[i]);
        ESP_ERROR_CHECK_WITHOUT_ABORT(ink_input_set_wait_mask(buttons[i], mask));
    }
}

/* The stack just changed: drop the pending continuation, let a pushed or replaced scene enter, update the wait masks, draw. */
static void show_top_scene(bool call_enter) {
    top_scene_t top;
    if (!get_top_scene(&top)) { return; }
    s_after_report = nullptr;
    if (call_enter) { top.scene->enter(top.state, false); }
    refresh_wait_masks();
    top.scene->draw(top.state);
}

/* Close the open app: stop it and forget its stack. */
static void close_app(void) {
    if (s_app == nullptr) { return; }
    /* TODO(system): call save() into the RTC slot of the app before closing; the slot belongs to the session block. */
    /* TODO(system): debug build: log when the heap after stop() differs from the heap before start() (leak check). */
    if (s_app->stop != nullptr) { s_app->stop(); }
    s_app          = nullptr;
    s_after_report = nullptr;
}

/* An app is usable when every pointer system calls is set and every scene table (over the app table) can reach BACK. */
static bool app_is_valid(const ink_system_app_t *app) {
    if (app == nullptr || app->name == nullptr || app->app_table == nullptr) { return false; }
    if (app->scenes == nullptr || app->scene_count == 0) { return false; }
    for (uint8_t i = 0; i < app->scene_count; i++) {
        const ink_system_scene_t *scene = &app->scenes[i];
        if (scene->enter == nullptr || scene->command == nullptr || scene->draw == nullptr) { return false; }
        if (ink_keymap_validate(scene->keymap, app->app_table) != INK_KEYMAP_OK) { return false; }
    }
    return true;
}

/* ---- Wake session ---- */
/* Design: docs/design/wake-session.md. ink_session decides; this part reads the clock, runs the idle timer, writes the
 * RTC slots, draws the background frame and calls deep sleep. All of it runs on the main task, except boot. */

#define SYSTEM_SLOT_VERSION 1    /* layout of sleep_metadata_t */
#define APP_SLOT_VERSION    1    /* layout of the app slot: ink_scene_snapshot_t, then app-level state */
#define BATTERY_UNKNOWN     0xFF /* battery_percent when the board cannot measure it */

/* What the background frame shows, kept in the system RTC slot; the frame is drawn again from it after wake. */
typedef struct {
    uint8_t  app_index;       /* app open before sleep */
    uint8_t  battery_percent; /* BATTERY_UNKNOWN when not measured */
    uint16_t due_count;       /* cards due for review */
} sleep_metadata_t;

static ink_session_state_t  s_session;
static ink_platform_timer_t s_idle_timer;
static uint64_t             s_wake_pin_mask;
static ink_ui_canvas_t      s_background_canvas; /* WARN: must not move after ink_ui_canvas_init() */
static bool                 s_restoring;         /* deep sleep wake with a valid system RTC slot; read by the boot message */
static sleep_metadata_t     s_restored;          /* metadata read after wake; valid when s_restoring */

static void post_idle(void *user_data) {
    (void)user_data;
    const message_t message = {.source = MESSAGE_SOURCE_IDLE};
    post_message(&message);
}

static uint32_t crc32(const uint8_t *data, size_t length) {
    return ink_platform_crc32(data, length);
}

/* Draw the background frame from metadata. Same firmware, same function, same numbers -> same pixels before sleep and
 * after wake, so wake does not flash.
 * TODO(system): temporary screen drawn with ui; replace with the Figma view. */
static void draw_background(uint8_t *frame, const sleep_metadata_t *metadata) {
    ink_ui_canvas_bind(&s_background_canvas, frame);
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(&s_background_canvas);
    ink_ui_fill(&s_background_canvas, bounds, 0, INK_UI_PAINT_WHITE);
    const ink_ui_text_style_t style = {.font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_CENTER};
    char                      text[64];
    snprintf(text, sizeof(text), "Đang ngủ\n\nThẻ cần ôn: %u\n\nBấm A để mở", (unsigned)metadata->due_count);
    /* TODO(system): draw the battery level once the board can measure it (battery_percent is BATTERY_UNKNOWN on BB0). */
    const ink_ui_size_t size = ink_ui_text_measure(&s_background_canvas, text, bounds.width, &style);
    const ink_ui_rect_t area = ink_ui_rect_align(bounds, size, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    ink_ui_text_draw(&s_background_canvas, area, text, &style, INK_UI_PAINT_BLACK);
    ink_ui_canvas_bind(&s_background_canvas, nullptr);
}

static uint8_t open_app_index(void) {
    for (uint8_t i = 0; i < s_app_count; i++) {
        if (s_apps[i] == s_app) { return i; }
    }
    return 0;
}

/* Write the scene stack and the app-level state of the open app into the app RTC slot. */
static void save_app_slot(void) {
    uint8_t *slot      = nullptr;
    size_t   slot_size = 0;
    if (s_app == nullptr || ink_platform_rtc_slot(INK_PLATFORM_RTC_SLOT_APP, &slot, &slot_size) != ESP_OK) { return; }
    uint8_t *payload = ink_session_slot_payload(slot, slot_size);
    if (payload == nullptr || slot_size - sizeof(ink_session_slot_header_t) < sizeof(ink_scene_snapshot_t)) { return; }
    const size_t         payload_size = slot_size - sizeof(ink_session_slot_header_t);
    ink_scene_snapshot_t scenes;
    ink_scene_save(&s_stack, &scenes);
    memcpy(payload, &scenes, sizeof(scenes));
    const size_t app_state_size = payload_size - sizeof(scenes);
    memset(payload + sizeof(scenes), 0, app_state_size);
    if (s_app->save != nullptr) { s_app->save(payload + sizeof(scenes), app_state_size); }
    if (ink_session_slot_seal(slot, slot_size, APP_SLOT_VERSION, payload_size, crc32) != INK_SESSION_OK) { ESP_LOGE(TAG, "seal app slot"); }
}

static void save_system_slot(const sleep_metadata_t *metadata) {
    uint8_t *slot      = nullptr;
    size_t   slot_size = 0;
    if (ink_platform_rtc_slot(INK_PLATFORM_RTC_SLOT_SYSTEM, &slot, &slot_size) != ESP_OK) { return; }
    uint8_t *payload = ink_session_slot_payload(slot, slot_size);
    if (payload == nullptr || slot_size - sizeof(ink_session_slot_header_t) < sizeof(*metadata)) { return; }
    memcpy(payload, metadata, sizeof(*metadata));
    if (ink_session_slot_seal(slot, slot_size, SYSTEM_SLOT_VERSION, sizeof(*metadata), crc32) != INK_SESSION_OK) {
        ESP_LOGE(TAG, "seal system slot");
    }
}

/* Read the system RTC slot after a deep sleep wake; false = slot invalid, boot as cold boot. */
static bool read_system_slot(sleep_metadata_t *out) {
    uint8_t *slot      = nullptr;
    size_t   slot_size = 0;
    size_t   length    = 0;
    if (ink_platform_rtc_slot(INK_PLATFORM_RTC_SLOT_SYSTEM, &slot, &slot_size) != ESP_OK) { return false; }
    if (!ink_session_slot_check(slot, slot_size, SYSTEM_SLOT_VERSION, crc32, &length) || length != sizeof(*out)) { return false; }
    memcpy(out, ink_session_slot_payload(slot, slot_size), sizeof(*out));
    return out->app_index < s_app_count;
}

/* Forget the system RTC slot: the next wake is handled as a cold boot. */
static void invalidate_system_slot(void) {
    uint8_t *slot      = nullptr;
    size_t   slot_size = 0;
    if (ink_platform_rtc_slot(INK_PLATFORM_RTC_SLOT_SYSTEM, &slot, &slot_size) == ESP_OK) { memset(slot, 0, slot_size); }
}

/* SAVE: every module writes its snapshot, then the background frame goes to display with BEFORE_SLEEP. Deep sleep
 * follows the POWER_OFF report of that frame. */
static void save_and_draw_background(void) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(ink_platform_timer_stop(s_idle_timer));
    s_after_report                  = nullptr; /* no second phase of a two-phase draw may cover the background frame */
    const sleep_metadata_t metadata = {
        .app_index       = open_app_index(),
        .battery_percent = BATTERY_UNKNOWN, /* TODO(system): measure the battery; BB0 has no measuring pin yet */
        .due_count       = 0,               /* TODO(system): count due cards through study once it exists */
    };
    save_app_slot();
    save_system_slot(&metadata);
    uint8_t *frame = ink_display_acquire();
    if (frame == nullptr) {
        ESP_LOGE(TAG, "acquire for the background frame: sleep without it, next wake is a cold boot");
        invalidate_system_slot();
        ink_platform_deep_sleep(s_wake_pin_mask);
    }
    draw_background(frame, &metadata);
    const esp_err_t error = ink_display_submit(frame, ink_platform_now_ms(), INK_DISPLAY_INTENT_PAGE, INK_DISPLAY_FLAG_BEFORE_SLEEP);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "submit of the background frame: %s", esp_err_to_name(error));
        invalidate_system_slot();
        ink_platform_deep_sleep(s_wake_pin_mask);
    }
    /* REVIEW(system): no time limit on the POWER_OFF report; a display task that hangs keeps the machine awake. */
}

/* Hand one event to the session and run the action it returns. */
static void run_session(ink_session_event_type_t type, uint32_t value) {
    const ink_session_event_t event = {.type = type, .time_ms = ink_platform_now_ms(), .value = value};
    ink_session_output_t      output;
    if (ink_session_handle(&s_session, &event, &output) != INK_SESSION_OK) { return; }
    switch (output.action) {
        case INK_SESSION_ACTION_ARM_TIMER:
            ESP_ERROR_CHECK_WITHOUT_ABORT(ink_platform_timer_start(s_idle_timer, output.delay_ms));
            break;
        case INK_SESSION_ACTION_SAVE:
            ESP_LOGI(TAG, "save and sleep");
            save_and_draw_background();
            break;
        case INK_SESSION_ACTION_DEEP_SLEEP:
            ESP_LOGI(TAG, "deep sleep");
            ink_platform_deep_sleep(s_wake_pin_mask);
        case INK_SESSION_ACTION_NONE:
        case INK_SESSION_ACTION_RESTORE: /* BOOT is handled in ink_system_start() */
        case INK_SESSION_ACTION_COLD_BOOT:
            break;
    }
}

/* ---- API for apps ---- */

/* Open app `app_index`. `snapshot` (scene stack and app-level state read after wake) may be nullptr: fresh open. */
static esp_err_t open_app(uint8_t app_index, const ink_scene_snapshot_t *scenes, const uint8_t *app_state, size_t app_state_length) {
    if (app_index >= s_app_count) { return ESP_ERR_INVALID_ARG; }
    close_app();
    const ink_system_app_t *app   = s_apps[app_index];
    const esp_err_t         error = (app->start != nullptr) ? app->start(app_state, app_state_length) : ESP_OK;
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "start of %s: %s", app->name, esp_err_to_name(error));
        if (app_index != 0) { ESP_ERROR_CHECK_WITHOUT_ABORT(ink_system_open_app(0)); }
        return error;
    }
    s_app = app;
    if (scenes == nullptr || ink_scene_restore(&s_stack, scenes, app->scene_count) != INK_SCENE_OK) {
        ink_scene_init(&s_stack, 0);
        show_top_scene(true);
        return ESP_OK;
    }
    /* Rebuilt after wake: enter with restored = true from the bottom up, then draw the top (docs/design/app-framework.md). */
    s_after_report = nullptr;
    for (uint8_t i = 0; i < ink_scene_depth(&s_stack); i++) {
        /* NOTE: s_stack is not const; ink_scene_entry_at() returns const only for read-only callers. */
        ink_scene_entry_t *entry = (ink_scene_entry_t *)ink_scene_entry_at(&s_stack, i);
        app->scenes[entry->scene_id].enter(entry->state, true);
    }
    show_top_scene(false);
    return ESP_OK;
}

esp_err_t ink_system_open_app(uint8_t app_index) {
    return open_app(app_index, nullptr, nullptr, 0);
}

esp_err_t ink_system_scene_push(uint8_t scene_id, const uint8_t *state) {
    if (s_app == nullptr) { return ESP_ERR_INVALID_STATE; }
    if (scene_id >= s_app->scene_count) { return ESP_ERR_INVALID_ARG; }
    const ink_scene_error_t error = ink_scene_push(&s_stack, scene_id, state);
    if (error != INK_SCENE_OK) { return to_esp_err(error); }
    show_top_scene(true);
    return ESP_OK;
}

esp_err_t ink_system_scene_pop(void) {
    if (s_app == nullptr) { return ESP_ERR_INVALID_STATE; }
    const ink_scene_error_t error = ink_scene_pop(&s_stack);
    if (error != INK_SCENE_OK) { return to_esp_err(error); }
    show_top_scene(false);
    return ESP_OK;
}

esp_err_t ink_system_scene_replace(uint8_t scene_id, const uint8_t *state) {
    if (s_app == nullptr) { return ESP_ERR_INVALID_STATE; }
    if (scene_id >= s_app->scene_count) { return ESP_ERR_INVALID_ARG; }
    const ink_scene_error_t error = ink_scene_replace(&s_stack, scene_id, state);
    if (error != INK_SCENE_OK) { return to_esp_err(error); }
    show_top_scene(true);
    return ESP_OK;
}

void ink_system_scene_reset(void) {
    if (s_app == nullptr) { return; }
    ink_scene_reset(&s_stack);
    show_top_scene(false);
}

uint8_t ink_system_app_count(void) {
    return s_app_count;
}

const ink_system_app_t *ink_system_app_at(uint8_t app_index) {
    return app_index < s_app_count ? s_apps[app_index] : nullptr;
}

void ink_system_keep_awake(uint32_t ms) {
    static_assert(INK_SYSTEM_KEEP_AWAKE_MAX_MS == INK_SESSION_KEEP_AWAKE_MAX_MS, "keep awake caps differ");
    run_session(INK_SESSION_EVENT_KEEP_AWAKE, ms);
}

void ink_system_sleep(void) {
    run_session(INK_SESSION_EVENT_SLEEP, 0);
}

void ink_system_after_report(void (*next)(const uint8_t *state)) {
    s_after_report = next;
}

/* ---- Gestures ---- */

static void open_home(void) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(ink_system_open_app(0));
}

/* A scene did not handle BACK: pop it; at the root of an app other than Home, go Home. BACK at the root of Home is ignored. */
static void go_back(void) {
    const bool at_root = ink_system_scene_pop() == ESP_ERR_INVALID_STATE;
    if (at_root && s_app != s_apps[0]) { open_home(); }
}

/* HOLD_AB_SHORT and HOLD_AB_LONG belong to system; every other gesture becomes a command for the scene on top. */
static void handle_gesture(const ink_gesture_t *gesture) {
    if (s_app == nullptr) { return; } /* a button pressed before the boot message was handled */
    /* Saving: nothing may draw over the background frame. */
    if (ink_session_phase(&s_session) != INK_SESSION_PHASE_ACTIVE) { return; }
    if (gesture->type == INK_GESTURE_TYPE_HOLD_AB_SHORT) {
        if (s_app != s_apps[0]) { open_home(); } /* already in Home: ignored (docs/design/input/input.md) */
        return;
    }
    if (gesture->type == INK_GESTURE_TYPE_HOLD_AB_LONG) {
        run_session(INK_SESSION_EVENT_SLEEP, 0);
        return;
    }
    top_scene_t top;
    if (!get_top_scene(&top)) { return; }
    ink_keymap_command_t command;
    if (!ink_keymap_lookup(top.scene->keymap, s_app->app_table, gesture, &command)) { return; }
    if (top.scene->command(top.state, command)) { return; }
    if (command == INK_KEYMAP_COMMAND_BACK) { go_back(); }
}

static void handle_gestures(const ink_gesture_output_t *gestures) {
    for (uint8_t i = 0; i < gestures->count; i++) { handle_gesture(&gestures->items[i]); }
}

/* ---- Display reports ---- */

/* Log the report, then run the continuation a scene left for it (second phase of a two-phase draw), once. */
static void handle_display_report(const ink_display_report_t *report) {
    ESP_LOGI(TAG, "display report: action %d, %s", report->action, esp_err_to_name(report->error));
    if (report->action == INK_DISPLAY_ACTION_POWER_OFF) { run_session(INK_SESSION_EVENT_POWER_OFF, 0); }
    void (*next)(const uint8_t *state) = s_after_report;
    if (next == nullptr || !report->is_frame) { return; } /* an idle cleanup or power off is not the frame the scene waits for */
    s_after_report = nullptr;
    top_scene_t top;
    if (get_top_scene(&top)) { next(top.state); }
}

/* ---- Boot message ---- */

/* Open the app of the snapshot after wake (app slot invalid -> fresh open of that app), or Home on cold boot; then
 * start the idle timer. */
static void handle_boot(void) {
    if (!s_restoring) {
        open_home();
    } else {
        uint8_t *slot      = nullptr;
        size_t   slot_size = 0;
        size_t   length    = 0;
        if (ink_platform_rtc_slot(INK_PLATFORM_RTC_SLOT_APP, &slot, &slot_size) == ESP_OK
            && ink_session_slot_check(slot, slot_size, APP_SLOT_VERSION, crc32, &length)
            && length >= sizeof(ink_scene_snapshot_t)) {
            const uint8_t       *payload = ink_session_slot_payload(slot, slot_size);
            ink_scene_snapshot_t scenes;
            memcpy(&scenes, payload, sizeof(scenes));
            ESP_ERROR_CHECK_WITHOUT_ABORT(open_app(s_restored.app_index, &scenes, payload + sizeof(scenes), length - sizeof(scenes)));
        } else {
            ESP_LOGW(TAG, "app RTC slot invalid: fresh open");
            ESP_ERROR_CHECK_WITHOUT_ABORT(open_app(s_restored.app_index, nullptr, nullptr, 0));
        }
    }
    run_session(INK_SESSION_EVENT_RESTORED, 0);
}

/* ---- Main task ---- */

/* WARN: stack must stay in internal RAM (xTaskCreatePinnedToCore default); deep sleep is called from this task.
 * REVIEW(system): raised from 4096 for the ui test screens (text layout on the stack); not measured. */
#define TASK_STACK_BYTES 8192
#define TASK_PRIORITY    5

static void task_main(void *user_data) {
    (void)user_data;
    message_t            message;
    ink_gesture_output_t gestures;
    bool                 messages_dropped = false; /* messages dropped, input not told yet */
    for (;;) {
        if (xQueueReceive(s_queue, &message, portMAX_DELAY) != pdTRUE) { continue; }
        const unsigned dropped_count = atomic_exchange(&s_dropped_count, 0);
        if (dropped_count) {
            ESP_LOGW(TAG, "main queue full: dropped %u messages", dropped_count);
            messages_dropped = true;
        }
        switch (message.source) {
            case MESSAGE_SOURCE_INPUT:
                /* Every press and release restarts the idle wait: a hold in progress is not a gesture yet, and the
                 * idle timer must not fire under a held button. */
                if (message.input.type == INK_INPUT_MESSAGE_BUTTON_EDGE) { run_session(INK_SESSION_EVENT_ACTIVITY, 0); }
                ink_input_handle(&message.input, &gestures);
                handle_gestures(&gestures);
                break;
            case MESSAGE_SOURCE_DISPLAY:
                handle_display_report(&message.display);
                break;
            case MESSAGE_SOURCE_BOOT:
                handle_boot();
                break;
            case MESSAGE_SOURCE_IDLE:
                run_session(INK_SESSION_EVENT_TIMER, 0);
                break;
        }
        if (uxQueueMessagesWaiting(s_queue) == 0) {
            ink_input_on_queue_empty(messages_dropped, &gestures);
            handle_gestures(&gestures);
            messages_dropped = false;
        }
    }
}

/* ---- Boot ---- */

static esp_err_t create_queue_task(void) {
    s_queue = xQueueCreate(QUEUE_LENGTH, sizeof(message_t));
    if (s_queue == nullptr) { return ESP_ERR_NO_MEM; }
    if (xTaskCreatePinnedToCore(task_main, "ink_main", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, nullptr, tskNO_AFFINITY) != pdPASS) {
        vQueueDelete(s_queue);
        s_queue = nullptr;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static bool apps_are_valid(const ink_system_config_t *config) {
    if (config->apps == nullptr || config->app_count == 0) { return false; }
    for (uint8_t i = 0; i < config->app_count; i++) {
        if (!app_is_valid(config->apps[i])) { return false; }
    }
    return true;
}

esp_err_t ink_system_start(const ink_system_config_t *config) {
    if (config == nullptr || config->display == nullptr || !apps_are_valid(config)) { return ESP_ERR_INVALID_ARG; }
    if (config->idle_ms == 0 || config->wake_pin_mask == 0) { return ESP_ERR_INVALID_ARG; }
    if (s_queue != nullptr) { return ESP_ERR_INVALID_STATE; }
    s_apps          = config->apps;
    s_app_count     = config->app_count;
    s_wake_pin_mask = config->wake_pin_mask;
    /* NOTE: a failure after create_queue_task() leaves the queue and task alive; no retry, because a boot error is
     * fatal (app_main aborts through ESP_ERROR_CHECK). */
    if (ink_session_init(&s_session, config->idle_ms) != INK_SESSION_OK) { return ESP_ERR_INVALID_ARG; }
    const ink_display_frame_info_t frame_info = {
        .width  = config->display->capabilities.width,
        .height = config->display->capabilities.height,
        .format = config->display->capabilities.pixel_format,
    };
    if (ink_ui_canvas_init(&s_background_canvas, &frame_info, INK_UI_ROTATION_0) != INK_UI_OK) { return ESP_ERR_NOT_SUPPORTED; }
    esp_err_t error = ink_platform_timer_create(post_idle, nullptr, "ink_idle", &s_idle_timer);
    if (error == ESP_OK) { error = create_queue_task(); }
    if (error == ESP_OK) { error = ink_input_start(config->buttons, config->button_count, post_input, post_input_from_isr); }

    /* Reset reason decides whether the RTC slots are read; only a deep sleep wake trusts them. */
    const ink_session_event_t boot = {
        .type    = INK_SESSION_EVENT_BOOT,
        .time_ms = ink_platform_now_ms(),
        .value   = ink_platform_woke_from_deep_sleep() ? 1 : 0,
    };
    ink_session_output_t boot_output;
    if (error == ESP_OK
        && ink_session_handle(&s_session, &boot, &boot_output) == INK_SESSION_OK
        && boot_output.action == INK_SESSION_ACTION_RESTORE) {
        s_restoring = read_system_slot(&s_restored);
        if (!s_restoring) { ESP_LOGW(TAG, "system RTC slot invalid: cold boot"); }
    }
    uint8_t *background = nullptr; /* rebuilt background frame; display copies it at start */
    if (s_restoring) {
        /* REVIEW(system): text layout runs on the app_main stack here (6144 B); not measured. */
        background = malloc(((size_t)frame_info.width + 7) / 8 * frame_info.height);
        if (background != nullptr) {
            draw_background(background, &s_restored);
        } else {
            ESP_LOGE(TAG, "no memory for the background frame: first draw is cleanup strong");
        }
    }
    ESP_LOGI(TAG, "boot: %s", s_restoring ? "deep sleep wake" : "cold boot");
    if (error == ESP_OK) {
        const ink_display_config_t display_config = {
            .device     = config->display,
            .background = background,
            .options    = config->display_options,
            .on_report  = post_display_report,
        };
        error = ink_display_start(&display_config);
    }
    free(background);
    if (error == ESP_OK) {
        const message_t message = {.source = MESSAGE_SOURCE_BOOT};
        post_message(&message);
    }
    return error;
}
