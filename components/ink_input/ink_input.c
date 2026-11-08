/* Design: docs/design/input/input.md */

/* Module state and the main task side: edge -> gesture, timer at deadlines, resync, gesture config. */
#include "esp_attr.h"
#include "esp_log.h"
#include "ink_input_internal.h"
#include "ink_platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char *TAG = "ink_input";

/* ---- Post functions from system ---- */
/* Set once in ink_input_start(), before interrupts and the timer run; read only afterwards. */

static ink_input_post_t          s_post;
static ink_input_post_from_isr_t s_post_from_isr;

/* ---- Message sources: button ISR, timer ---- */

/* user_data is the button code (ink_gesture_button_t). The timestamp is taken here, so there is one time source. */
static void IRAM_ATTR on_button_edge(void *user_data, bool pressed) {
    ink_input_message_t message = {.type = INK_INPUT_MESSAGE_BUTTON_EDGE};

    message.button_edge = (ink_gesture_event_t){
        .button  = (ink_gesture_button_t)(uintptr_t)user_data,
        .level   = pressed ? INK_GESTURE_LEVEL_PRESSED : INK_GESTURE_LEVEL_RELEASED,
        .time_ms = ink_platform_now_ms(),
    };
    s_post_from_isr(&message);
}

static void on_timer_expired(void *user_data) {
    (void)user_data;
    const ink_input_message_t message = {.type = INK_INPUT_MESSAGE_TIMER_EXPIRED};
    s_post(&message);
}

/* ---- Main task state ---- */
/* Only the main task reads or writes these. */

/* REVIEW(input): defaults not measured on BB0 buttons; move to options when Settings exists. */
static const ink_gesture_config_t s_default_gesture_config = {
    .debounce_ms = 20,
    .double_ms   = 400,
    .hold_ms     = 600,
    .combo_ms    = 100,
    .ab_short_ms = 2000,
    .ab_long_ms  = 30000,
};

static ink_gesture_state_t            s_gesture_state;
static ink_platform_timer_t           s_gesture_timer;
static const ink_hal_button_device_t *s_buttons;                                /* index 0 = A, 1 = B; NULL until started */
static bool                           s_last_pressed[INK_GESTURE_BUTTON_COUNT]; /* level of the last edge fed to gesture */
static bool                           s_resync_pending;                         /* timer fired; resync not done yet */

/* ---- Timer at gesture deadlines ---- */

#define PROGRESS_LOG_INTERVAL_MS 1000

/*
 * Start the timer at the earliest deadline; while a gesture is in progress, at most PROGRESS_LOG_INTERVAL_MS away.
 * Log progress only when called from the timer: from edges, one bounce would print dozens of lines.
 */
static void update_gesture_timer(bool log) {
    const uint32_t         now_ms = ink_platform_now_ms();
    ink_gesture_progress_t progress;
    const bool             has_progress = ink_gesture_progress(&s_gesture_state, now_ms, &progress);
    if (has_progress && log) { input_log_progress(&progress, now_ms); }

    uint32_t   deadline_ms  = 0;
    const bool has_deadline = ink_gesture_next_deadline(&s_gesture_state, &deadline_ms);
    if (!has_deadline && !has_progress) {
        ESP_ERROR_CHECK(ink_platform_timer_stop(s_gesture_timer));
        return;
    }
    uint32_t delay_ms = UINT32_MAX;
    if (has_deadline) {
        const int32_t remaining_ms = (int32_t)(deadline_ms - now_ms);
        delay_ms                   = remaining_ms > 0 ? (uint32_t)remaining_ms : 1;
    }
    if (has_progress && delay_ms > PROGRESS_LOG_INTERVAL_MS) { delay_ms = PROGRESS_LOG_INTERVAL_MS; }
    ESP_ERROR_CHECK(ink_platform_timer_start(s_gesture_timer, delay_ms));
}

/* ---- Edges into gesture ---- */

/* Append gestures to out; when out is full, drop the rest and log it. */
static void append_gestures(ink_gesture_output_t *out, const ink_gesture_output_t *gestures) {
    for (uint8_t i = 0; i < gestures->count; i++) {
        if (out->count == INK_GESTURE_OUTPUT_MAX) {
            ESP_LOGW(TAG, "dropped %u gestures: output full", gestures->count - i);
            return;
        }
        out->items[out->count++] = gestures->items[i];
    }
}

static void feed_edge(const ink_gesture_event_t *event, ink_gesture_output_t *out) {
    ink_gesture_output_t gestures = {0};
    if (ink_gesture_feed(&s_gesture_state, event, &gestures) != INK_GESTURE_OK) {
        ESP_LOGE(TAG, "feed button %u", event->button);
        return;
    }
    s_last_pressed[ink_gesture_index_of_button(event->button)] = event->level == INK_GESTURE_LEVEL_PRESSED;
    input_log_gestures(&gestures);
    append_gestures(out, &gestures);
}

/*
 * The last edge of a bounce can be dropped when the main queue is full; gesture then keeps a wrong level until the
 * next press. Read the real pin; if it differs from the last edge, feed a resync edge. Call only with the main queue
 * empty: while edges are queued, the pin level is not final.
 * Returns true if a resync edge was fed.
 */
static bool resync_buttons(ink_gesture_output_t *out) {
    bool fed = false;
    for (size_t i = 0; i < INK_GESTURE_BUTTON_COUNT; i++) {
        bool pressed = false;
        if (s_buttons[i].operations->read(s_buttons[i].context, &pressed) != ESP_OK) { continue; }
        if (pressed == s_last_pressed[i]) { continue; }
        const ink_gesture_event_t event = {
            .button  = ink_gesture_button_of_index((uint8_t)i),
            .level   = pressed ? INK_GESTURE_LEVEL_PRESSED : INK_GESTURE_LEVEL_RELEASED,
            .time_ms = ink_platform_now_ms(),
        };
        ESP_LOGW(TAG, "button %s: edge lost, feeding resync edge", input_button_name(event.button));
        feed_edge(&event, out);
        fed = true;
    }
    return fed;
}

/* ---- API for the main task ---- */

void ink_input_handle(const ink_input_message_t *message, ink_gesture_output_t *out_gestures) {
    out_gestures->count = 0;
    switch (message->type) {
        case INK_INPUT_MESSAGE_BUTTON_EDGE:
            feed_edge(&message->button_edge, out_gestures);
            break;
        case INK_INPUT_MESSAGE_TIMER_EXPIRED: {
            ink_gesture_output_t gestures = {0};
            if (ink_gesture_tick(&s_gesture_state, ink_platform_now_ms(), &gestures) != INK_GESTURE_OK) { ESP_LOGE(TAG, "tick"); }
            input_log_gestures(&gestures);
            append_gestures(out_gestures, &gestures);
            s_resync_pending = true;
            break;
        }
    }
    update_gesture_timer(message->type == INK_INPUT_MESSAGE_TIMER_EXPIRED);
}

esp_err_t ink_input_set_gesture_config(const ink_gesture_config_t *config) {
    if (s_buttons == nullptr) { return ESP_ERR_INVALID_STATE; }
    if (config == nullptr || ink_gesture_set_config(&s_gesture_state, config) != INK_GESTURE_OK) { return ESP_ERR_INVALID_ARG; }
    update_gesture_timer(false);
    return ESP_OK;
}

esp_err_t ink_input_set_wait_mask(ink_gesture_button_t button, ink_gesture_wait_mask_t wait_mask) {
    if (s_buttons == nullptr) { return ESP_ERR_INVALID_STATE; }
    if (ink_gesture_set_wait_mask(&s_gesture_state, button, wait_mask) != INK_GESTURE_OK) { return ESP_ERR_INVALID_ARG; }
    update_gesture_timer(false);
    return ESP_OK;
}

void ink_input_on_queue_empty(bool messages_dropped, ink_gesture_output_t *out_gestures) {
    out_gestures->count = 0;
    if (!messages_dropped && !s_resync_pending) { return; }
    s_resync_pending = false;
    if (resync_buttons(out_gestures)) { update_gesture_timer(false); }
}

/* ---- Boot ---- */

static esp_err_t connect_buttons(const ink_hal_button_device_t *buttons) {
    for (size_t i = 0; i < INK_GESTURE_BUTTON_COUNT; i++) {
        const ink_gesture_button_t button = ink_gesture_button_of_index((uint8_t)i);
        const esp_err_t error = buttons[i].operations->set_callback(buttons[i].context, on_button_edge, (void *)(uintptr_t)button);
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "button %s: %s", input_button_name(button), esp_err_to_name(error));
            return error;
        }
    }
    return ESP_OK;
}

esp_err_t ink_input_start(const ink_hal_button_device_t *buttons, size_t count, ink_input_post_t post,
                          ink_input_post_from_isr_t post_from_isr) {
    if (s_buttons != nullptr) { return ESP_ERR_INVALID_STATE; }
    if (buttons == nullptr || count != INK_GESTURE_BUTTON_COUNT || post == nullptr || post_from_isr == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < count; i++) {
        if (!buttons[i].capabilities.has_interrupt) { return ESP_ERR_NOT_SUPPORTED; }
    }
    if (ink_gesture_init(&s_gesture_state, &s_default_gesture_config) != INK_GESTURE_OK) { return ESP_ERR_INVALID_ARG; }
    s_post          = post;
    s_post_from_isr = post_from_isr;

    esp_err_t error = ink_platform_timer_create(on_timer_expired, nullptr, "gesture", &s_gesture_timer);
    if (error != ESP_OK) { return error; }
    s_buttons = buttons;
    /* FIXME(input): a connect_buttons() failure leaves s_buttons set and the timer allocated; a retry returns
     * ESP_ERR_INVALID_STATE. */
    return connect_buttons(buttons);
}
