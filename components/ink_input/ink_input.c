/* Thiết kế: docs/design/input.md */

/* Trạng thái của khối và phần chạy trên task main: edge → gesture, timer theo deadline, đọc lại chân, config. */
#include "esp_attr.h"
#include "esp_log.h"
#include "ink_input_internal.h"
#include "ink_platform.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char *TAG = "ink_input";

#define BUTTON_COUNT 2

/* ---- Hàm post của system ---- */
/* Gán một lần trong ink_input_start(), trước khi bật ngắt và timer; sau đó chỉ đọc. */

static ink_input_post_t          s_post;
static ink_input_post_from_isr_t s_post_from_isr;

void input_post(const ink_input_message_t *message) {
    s_post(message);
}

/* ---- Nguồn message: ISR nút, timer ---- */

/* user_data là mã nút (ink_gesture_button_t). Đóng dấu thời gian tại đây: chỉ một nguồn giờ. */
static void IRAM_ATTR on_button_edge(void *user_data, bool pressed) {
    ink_input_message_t message = {.type = INK_INPUT_MESSAGE_BUTTON_EDGE};
    message.button_edge         = (ink_gesture_event_t){
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

/* ---- Trạng thái task main ---- */
/* Chỉ task main đọc và sửa. */

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
static const ink_hal_button_device_t *s_buttons;                    /* index 0 là A, 1 là B; nullptr khi chưa start */
static bool                           s_last_pressed[BUTTON_COUNT]; /* level của edge cuối đã đưa vào gesture */
static bool                           s_resync_pending;             /* timer vừa chạy, chưa đọc lại chân */

bool input_is_started(void) {
    return s_buttons != nullptr;
}

/* ---- Timer theo deadline của gesture ---- */

#define PROGRESS_LOG_INTERVAL_MS 1000

/*
 * Hẹn timer theo deadline gần nhất; có gesture dở thì hẹn không quá PROGRESS_LOG_INTERVAL_MS.
 * Chỉ log progress khi gọi từ timer: gọi từ edge thì một chùm rung in vài chục dòng.
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

/* ---- Config gesture từ lệnh console ---- */

static void show_config(void) {
    ink_gesture_config_t config;
    if (ink_gesture_get_config(&s_gesture_state, &config) != INK_GESTURE_OK) { return; }
    for (size_t i = 0; i < input_config_field_count(); i++) {
        ESP_LOGI(TAG, "%s = %lu", input_config_field_name(i), (unsigned long)*input_config_field_value(&config, i));
    }
}

static void set_config_field(size_t field_index, uint32_t value_ms) {
    ink_gesture_config_t config;
    if (ink_gesture_get_config(&s_gesture_state, &config) != INK_GESTURE_OK) { return; }
    *input_config_field_value(&config, field_index) = value_ms;
    if (ink_gesture_set_config(&s_gesture_state, &config) != INK_GESTURE_OK) {
        ESP_LOGW(TAG, "config sai, giữ config cũ");
        return;
    }
    ESP_LOGI(TAG, "%s = %lu", input_config_field_name(field_index), (unsigned long)value_ms);
}

static void set_wait_mask(ink_gesture_button_t button, uint8_t wait_mask) {
    if (ink_gesture_set_enabled(&s_gesture_state, button, wait_mask) != INK_GESTURE_OK) {
        ESP_LOGW(TAG, "mask sai");
        return;
    }
    ESP_LOGI(TAG, "mask %s = %u", input_button_name(button), wait_mask);
}

/* ---- Edge vào gesture ---- */

/* Gom gesture vào out; out đầy thì bỏ phần thừa và báo. */
static void append_gestures(ink_gesture_output_t *out, const ink_gesture_output_t *gestures) {
    for (uint8_t i = 0; i < gestures->count; i++) {
        if (out->count == INK_GESTURE_OUTPUT_MAX) {
            ESP_LOGW(TAG, "bỏ %u gesture vì đầy output", gestures->count - i);
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
    s_last_pressed[event->button == INK_GESTURE_BUTTON_A ? 0 : 1] = event->level == INK_GESTURE_LEVEL_PRESSED;
    input_log_gestures(&gestures);
    append_gestures(out, &gestures);
}

/*
 * Edge cuối của một chùm rung có thể rơi khi hàng đợi đầy; gesture giữ level sai tới lần bấm sau. Đọc chân thật,
 * khác level edge cuối thì đưa edge bù. Chỉ gọi khi hàng đợi rỗng: còn edge chưa xử lý thì level chân chưa phải level cuối.
 * Trả true nếu đã đưa edge bù.
 */
static bool resync_buttons(ink_gesture_output_t *out) {
    bool fed = false;
    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        bool pressed = false;
        if (s_buttons[i].operations->read(s_buttons[i].context, &pressed) != ESP_OK) { continue; }
        if (pressed == s_last_pressed[i]) { continue; }
        const ink_gesture_event_t event = {
            .button  = (ink_gesture_button_t)(1u << i),
            .level   = pressed ? INK_GESTURE_LEVEL_PRESSED : INK_GESTURE_LEVEL_RELEASED,
            .time_ms = ink_platform_now_ms(),
        };
        ESP_LOGW(TAG, "button %s: mất edge, đưa edge bù", input_button_name(event.button));
        feed_edge(&event, out);
        fed = true;
    }
    return fed;
}

/* ---- API cho task main ---- */

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
        case INK_INPUT_MESSAGE_SHOW_CONFIG:
            show_config();
            break;
        case INK_INPUT_MESSAGE_SET_CONFIG:
            set_config_field(message->set_config.field_index, message->set_config.value_ms);
            break;
        case INK_INPUT_MESSAGE_SET_WAIT_MASK:
            set_wait_mask(message->set_wait_mask.button, message->set_wait_mask.wait_mask);
            break;
    }
    update_gesture_timer(message->type == INK_INPUT_MESSAGE_TIMER_EXPIRED);
}

void ink_input_on_queue_empty(bool messages_dropped, ink_gesture_output_t *out_gestures) {
    out_gestures->count = 0;
    if (!messages_dropped && !s_resync_pending) { return; }
    s_resync_pending = false;
    if (resync_buttons(out_gestures)) { update_gesture_timer(false); }
}

/* ---- Khởi động ---- */

/* Nút index i có mã bit i (A = 1, B = 2). */
static esp_err_t connect_buttons(const ink_hal_button_device_t *buttons) {
    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        const ink_gesture_button_t button = (ink_gesture_button_t)(1u << i);
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
    if (buttons == nullptr || count != BUTTON_COUNT || post == nullptr || post_from_isr == nullptr) { return ESP_ERR_INVALID_ARG; }
    for (size_t i = 0; i < count; i++) {
        if (!buttons[i].capabilities.has_interrupt) { return ESP_ERR_NOT_SUPPORTED; }
    }
    if (ink_gesture_init(&s_gesture_state, &s_default_gesture_config) != INK_GESTURE_OK) { return ESP_ERR_INVALID_ARG; }
    s_post          = post;
    s_post_from_isr = post_from_isr;

    esp_err_t error = ink_platform_timer_create(on_timer_expired, nullptr, "gesture", &s_gesture_timer);
    if (error != ESP_OK) { return error; }
    s_buttons = buttons;
    return connect_buttons(buttons);
}
