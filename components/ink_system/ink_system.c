/* Design: docs/design/tasks.md */
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
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "ink_system";

/* ---- Messages to the main task ---- */
/* ISRs and timers of every module only build messages; all work happens on the main task. */

typedef enum : uint8_t {
    MESSAGE_SOURCE_INPUT = 0,
    MESSAGE_SOURCE_DISPLAY, /* report of the display task: a frame was drawn, a cleanup ran... */
    MESSAGE_SOURCE_BOOT,    /* boot finished: open Home on the main task (apps draw only from the main task) */
    /* EXTEND(system): one source per module that posts messages (session, storage...). */
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

/* ---- API for apps ---- */

esp_err_t ink_system_open_app(uint8_t app_index) {
    if (app_index >= s_app_count) { return ESP_ERR_INVALID_ARG; }
    close_app();
    const ink_system_app_t *app = s_apps[app_index];
    /* TODO(system): pass the snapshot of the app after wake; every open is a fresh open until the session block exists. */
    const esp_err_t error = (app->start != nullptr) ? app->start(nullptr, 0) : ESP_OK;
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "start of %s: %s", app->name, esp_err_to_name(error));
        if (app_index != 0) { ESP_ERROR_CHECK_WITHOUT_ABORT(ink_system_open_app(0)); }
        return error;
    }
    s_app = app;
    ink_scene_init(&s_stack, 0);
    show_top_scene(true);
    return ESP_OK;
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
    /* TODO(system): hand the request (capped at INK_SYSTEM_KEEP_AWAKE_MAX_MS) to the session block; no session yet. */
    (void)ms;
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
    if (gesture->type == INK_GESTURE_TYPE_HOLD_AB_SHORT) {
        if (s_app != s_apps[0]) { open_home(); } /* already in Home: ignored (docs/design/input/input.md) */
        return;
    }
    if (gesture->type == INK_GESTURE_TYPE_HOLD_AB_LONG) {
        /* TODO(system): power off through the session block (background frame, deep sleep); not built yet. */
        ESP_LOGW(TAG, "power off requested: session not built");
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
    void (*next)(const uint8_t *state) = s_after_report;
    if (next == nullptr || !report->is_frame) { return; } /* an idle cleanup or power off is not the frame the scene waits for */
    s_after_report = nullptr;
    top_scene_t top;
    if (get_top_scene(&top)) { next(top.state); }
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
                ink_input_handle(&message.input, &gestures);
                handle_gestures(&gestures);
                break;
            case MESSAGE_SOURCE_DISPLAY:
                handle_display_report(&message.display);
                break;
            case MESSAGE_SOURCE_BOOT:
                open_home();
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
    if (s_queue != nullptr) { return ESP_ERR_INVALID_STATE; }
    s_apps      = config->apps;
    s_app_count = config->app_count;
    /* NOTE: a failure after create_queue_task() leaves the queue and task alive; no retry, because a boot error is
     * fatal (app_main aborts through ESP_ERROR_CHECK). */
    esp_err_t error = create_queue_task();
    if (error == ESP_OK) { error = ink_input_start(config->buttons, config->button_count, post_input, post_input_from_isr); }
    /* TODO(system): no snapshot yet; every boot is a cold boot (no background frame, Home opens). */
    if (error == ESP_OK) {
        const ink_display_config_t display_config = {
            .device    = config->display,
            .options   = config->display_options,
            .on_report = post_display_report,
        };
        error = ink_display_start(&display_config);
    }
    if (error == ESP_OK) {
        const message_t message = {.source = MESSAGE_SOURCE_BOOT};
        post_message(&message);
    }
    return error;
}
