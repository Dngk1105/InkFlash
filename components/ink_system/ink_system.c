/* Thiết kế: docs/design/input.md */
#include "ink_system.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_gesture.h"
#include "ink_platform.h"

static const char *TAG = "ink_system";

/* ---- Message vào task main ---- */
/* ISR nút, timer và console chỉ tạo message; mọi việc làm trên task main (docs/design/tasks.md). */

typedef enum : uint8_t {
    MESSAGE_BUTTON_EDGE = 0,
    MESSAGE_TIMER_EXPIRED,
    MESSAGE_SHOW_CONFIG,
    MESSAGE_SET_CONFIG,
    MESSAGE_SET_WAIT_MASK,
} message_type_t;

typedef struct {
    message_type_t type;

    union {
        ink_gesture_event_t button_edge;

        struct {
            uint8_t  field_index; /* index trong s_config_fields */
            uint32_t value_ms;
        } set_config;

        struct {
            ink_gesture_button_t button;
            uint8_t              wait_mask; /* bit INK_GESTURE_WAIT_* */
        } set_wait_mask;
    };
} message_t;

/* ---- Hàng đợi task main ---- */

#define QUEUE_LENGTH 32 /* đủ cho một chùm edge rung của cả hai nút */

static QueueHandle_t s_queue;
static atomic_uint   s_dropped_count; /* message rơi vì hàng đợi đầy; ISR và task phụ cộng, task main đọc */

/* Gọi từ task phụ (timer, console), không chờ. */
static void post_message(const message_t *message)
{
    if (xQueueSend(s_queue, message, 0) != pdTRUE) { atomic_fetch_add(&s_dropped_count, 1); }
}

static void IRAM_ATTR post_message_from_isr(const message_t *message)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    if (xQueueSendFromISR(s_queue, message, &higher_priority_task_woken) != pdTRUE) {
        atomic_fetch_add(&s_dropped_count, 1);
    }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/* ---- Nguồn message: ISR nút, timer ---- */

/* user_data là mã nút (ink_gesture_button_t). Đóng dấu thời gian tại đây: chỉ một nguồn giờ. */
static void IRAM_ATTR on_button_edge(void *user_data, bool pressed)
{
    message_t message   = {.type = MESSAGE_BUTTON_EDGE};
    message.button_edge = (ink_gesture_event_t){
        .button  = (ink_gesture_button_t)(uintptr_t)user_data,
        .level   = pressed ? INK_GESTURE_LEVEL_PRESSED : INK_GESTURE_LEVEL_RELEASED,
        .time_ms = ink_platform_now_ms(),
    };
    post_message_from_isr(&message);
}

static void on_timer_expired(void *user_data)
{
    (void)user_data;
    const message_t message = {.type = MESSAGE_TIMER_EXPIRED};
    post_message(&message);
}

/* ---- Bảng trường config gesture ---- */
/* Dùng chung cho lệnh console (tìm theo tên) và task main (đọc, ghi theo offset). */

typedef struct {
    const char *name;
    size_t      offset;
} config_field_t;

static const config_field_t s_config_fields[] = {
    {"debounce_ms", offsetof(ink_gesture_config_t, debounce_ms)},
    {"double_ms",   offsetof(ink_gesture_config_t, double_ms)  },
    {"hold_ms",     offsetof(ink_gesture_config_t, hold_ms)    },
    {"combo_ms",    offsetof(ink_gesture_config_t, combo_ms)   },
    {"ab_short_ms", offsetof(ink_gesture_config_t, ab_short_ms)},
    {"ab_long_ms",  offsetof(ink_gesture_config_t, ab_long_ms) },
};

#define CONFIG_FIELD_COUNT (sizeof(s_config_fields) / sizeof(s_config_fields[0]))

static uint32_t *config_field_value(ink_gesture_config_t *config, size_t field_index)
{
    return (uint32_t *)((uint8_t *)config + s_config_fields[field_index].offset);
}

/* ---- Lệnh console `gesture` ---- */
/* Chạy trên task REPL: chỉ kiểm tra cú pháp rồi đẩy message; task main làm và log kết quả. */

static bool parse_uint32(const char *text, uint32_t *out_value)
{
    char         *end   = nullptr;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value > UINT32_MAX) { return false; }
    *out_value = (uint32_t)value;
    return true;
}

static bool parse_set_config(char **argv, message_t *out_message)
{
    size_t field_index = 0;
    while (field_index < CONFIG_FIELD_COUNT && strcmp(argv[2], s_config_fields[field_index].name) != 0) {
        field_index++;
    }
    if (field_index == CONFIG_FIELD_COUNT || !parse_uint32(argv[3], &out_message->set_config.value_ms)) {
        printf("gesture set <debounce_ms|double_ms|hold_ms|combo_ms|ab_short_ms|ab_long_ms> <ms>\n");
        return false;
    }
    out_message->type                   = MESSAGE_SET_CONFIG;
    out_message->set_config.field_index = (uint8_t)field_index;
    return true;
}

static bool parse_set_wait_mask(char **argv, message_t *out_message)
{
    const bool is_button_a = strcmp(argv[2], "a") == 0;
    const bool is_button_b = strcmp(argv[2], "b") == 0;
    uint32_t   wait_mask   = 0;
    if (!(is_button_a || is_button_b) || !parse_uint32(argv[3], &wait_mask) || wait_mask > UINT8_MAX) {
        printf("gesture enable <a|b> <mask>  (1 = double, 2 = modifier)\n");
        return false;
    }
    out_message->type                    = MESSAGE_SET_WAIT_MASK;
    out_message->set_wait_mask.button    = is_button_a ? INK_GESTURE_BUTTON_A : INK_GESTURE_BUTTON_B;
    out_message->set_wait_mask.wait_mask = (uint8_t)wait_mask;
    return true;
}

static int handle_gesture_command(int argc, char **argv)
{
    message_t message = {0};
    bool      valid   = false;
    if (argc == 2 && strcmp(argv[1], "show") == 0) {
        message.type = MESSAGE_SHOW_CONFIG;
        valid        = true;
    } else if (argc == 4 && strcmp(argv[1], "set") == 0) {
        valid = parse_set_config(argv, &message);
    } else if (argc == 4 && strcmp(argv[1], "enable") == 0) {
        valid = parse_set_wait_mask(argv, &message);
    } else {
        printf("gesture show | set <field> <ms> | enable <a|b> <mask>\n");
    }
    if (!valid) { return 1; }
    post_message(&message);
    return 0;
}

static esp_err_t start_console(void)
{
    esp_err_t error = ink_platform_console_init("ink> ");
    if (error == ESP_OK) {
        error = ink_platform_console_add("gesture", "Xem, đổi ngưỡng gesture và mask kiểu phải chờ",
                                         "show | set <field> <ms> | enable <a|b> <mask>", handle_gesture_command);
    }
    if (error == ESP_OK) { error = ink_platform_console_start(); }
    return error;
}

/* ---- Trạng thái task main ---- */
/* Chỉ task_main đọc và sửa. */

static const ink_gesture_config_t s_default_gesture_config = {
    .debounce_ms = 20,
    .double_ms   = 400,
    .hold_ms     = 600,
    .combo_ms    = 100,
    .ab_short_ms = 2000,
    .ab_long_ms  = 30000,
};

static ink_gesture_state_t  s_gesture_state;
static ink_platform_timer_t s_gesture_timer;

/* ---- Log gesture và progress ---- */

static const char *button_name(uint8_t button)
{
    switch (button) {
        case INK_GESTURE_BUTTON_A:
            return "A";
        case INK_GESTURE_BUTTON_B:
            return "B";
        case INK_GESTURE_BUTTON_AB:
            return "AB";
        default:
            return "-";
    }
}

static const char *gesture_type_name(ink_gesture_type_t type)
{
    switch (type) {
        case INK_GESTURE_TYPE_PRESS:
            return "press";
        case INK_GESTURE_TYPE_DOUBLE_PRESS:
            return "double";
        case INK_GESTURE_TYPE_HOLD:
            return "hold";
        case INK_GESTURE_TYPE_HOLD_AB_SHORT:
            return "hold_ab_short";
        case INK_GESTURE_TYPE_HOLD_AB_LONG:
            return "hold_ab_long";
        default:
            return "?";
    }
}

static void log_gestures(const ink_gesture_output_t *gestures)
{
    for (uint8_t i = 0; i < gestures->count; i++) {
        const ink_gesture_t *gesture = &gestures->items[i];
        ESP_LOGI(TAG, "gesture modifier=%s button=%s type=%s", button_name(gesture->modifier),
                 button_name(gesture->button), gesture_type_name(gesture->type));
    }
}

static void log_progress(const ink_gesture_progress_t *progress, uint32_t now_ms)
{
    const unsigned long held_ms = now_ms - progress->start_ms;
    if (progress->has_next) {
        ESP_LOGI(TAG, "progress modifier=%s button=%s held=%lu ms release=%s next=%s in %ld ms",
                 button_name(progress->modifier), button_name(progress->button), held_ms,
                 gesture_type_name(progress->on_release), gesture_type_name(progress->next),
                 (long)(int32_t)(progress->next_ms - now_ms));
    } else {
        ESP_LOGI(TAG, "progress modifier=%s button=%s held=%lu ms release=%s", button_name(progress->modifier),
                 button_name(progress->button), held_ms, gesture_type_name(progress->on_release));
    }
}

/* ---- Timer theo deadline của gesture ---- */

#define PROGRESS_LOG_INTERVAL_MS 1000

/* Log progress, hẹn timer theo deadline gần nhất; có gesture dở thì hẹn không quá PROGRESS_LOG_INTERVAL_MS. */
static void update_gesture_timer(void)
{
    const uint32_t         now_ms = ink_platform_now_ms();
    ink_gesture_progress_t progress;
    const bool             has_progress = ink_gesture_progress(&s_gesture_state, now_ms, &progress);
    if (has_progress) { log_progress(&progress, now_ms); }

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

static void show_config(void)
{
    ink_gesture_config_t config;
    if (ink_gesture_get_config(&s_gesture_state, &config) != INK_GESTURE_OK) { return; }
    for (size_t i = 0; i < CONFIG_FIELD_COUNT; i++) {
        ESP_LOGI(TAG, "%s = %lu", s_config_fields[i].name, (unsigned long)*config_field_value(&config, i));
    }
}

static void set_config_field(size_t field_index, uint32_t value_ms)
{
    ink_gesture_config_t config;
    if (ink_gesture_get_config(&s_gesture_state, &config) != INK_GESTURE_OK) { return; }
    *config_field_value(&config, field_index) = value_ms;
    if (ink_gesture_set_config(&s_gesture_state, &config) != INK_GESTURE_OK) {
        ESP_LOGW(TAG, "config sai, giữ config cũ");
        return;
    }
    ESP_LOGI(TAG, "%s = %lu", s_config_fields[field_index].name, (unsigned long)value_ms);
}

static void set_wait_mask(ink_gesture_button_t button, uint8_t wait_mask)
{
    if (ink_gesture_set_enabled(&s_gesture_state, button, wait_mask) != INK_GESTURE_OK) {
        ESP_LOGW(TAG, "mask sai");
        return;
    }
    ESP_LOGI(TAG, "mask %s = %u", button_name(button), wait_mask);
}

/* ---- Task main ---- */

#define TASK_STACK_BYTES 4096 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY    5

static void handle_message(const message_t *message)
{
    ink_gesture_output_t gestures = {0};
    switch (message->type) {
        case MESSAGE_BUTTON_EDGE:
            if (ink_gesture_feed(&s_gesture_state, &message->button_edge, &gestures) != INK_GESTURE_OK) {
                ESP_LOGE(TAG, "feed button %u", message->button_edge.button);
            }
            break;
        case MESSAGE_TIMER_EXPIRED:
            if (ink_gesture_tick(&s_gesture_state, ink_platform_now_ms(), &gestures) != INK_GESTURE_OK) {
                ESP_LOGE(TAG, "tick");
            }
            break;
        case MESSAGE_SHOW_CONFIG:
            show_config();
            break;
        case MESSAGE_SET_CONFIG:
            set_config_field(message->set_config.field_index, message->set_config.value_ms);
            break;
        case MESSAGE_SET_WAIT_MASK:
            set_wait_mask(message->set_wait_mask.button, message->set_wait_mask.wait_mask);
            break;
    }
    log_gestures(&gestures);
    update_gesture_timer();
}

static void task_main(void *user_data)
{
    (void)user_data;
    message_t message;
    for (;;) {
        if (xQueueReceive(s_queue, &message, portMAX_DELAY) != pdTRUE) { continue; }
        const unsigned dropped_count = atomic_exchange(&s_dropped_count, 0);
        if (dropped_count) { ESP_LOGW(TAG, "hàng đợi đầy, rơi %u message", dropped_count); }
        handle_message(&message);
    }
}

/* ---- Khởi động ---- */

#define BUTTON_COUNT 2

static esp_err_t create_queue_timer_task(void)
{
    s_queue = xQueueCreate(QUEUE_LENGTH, sizeof(message_t));
    if (s_queue == nullptr) { return ESP_ERR_NO_MEM; }
    esp_err_t error = ink_platform_timer_create(on_timer_expired, nullptr, "gesture", &s_gesture_timer);
    if (error == ESP_OK
        && xTaskCreatePinnedToCore(task_main, "ink_main", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, nullptr,
                                   tskNO_AFFINITY)
            != pdPASS) {
        error = ESP_ERR_NO_MEM;
    }
    if (error != ESP_OK) {
        vQueueDelete(s_queue);
        s_queue = nullptr;
    }
    return error;
}

/* Nút index i có mã bit i (A = 1, B = 2). */
static esp_err_t connect_buttons(const ink_hal_button_device_t *buttons)
{
    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        const ink_gesture_button_t button = (ink_gesture_button_t)(1u << i);
        const esp_err_t            error =
            buttons[i].operations->set_callback(buttons[i].context, on_button_edge, (void *)(uintptr_t)button);
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "button %s: %s", button_name(button), esp_err_to_name(error));
            return error;
        }
    }
    return ESP_OK;
}

esp_err_t ink_system_start(const ink_hal_button_device_t *buttons, size_t count)
{
    if (s_queue != nullptr) { return ESP_ERR_INVALID_STATE; }
    if (buttons == nullptr || count != BUTTON_COUNT) { return ESP_ERR_INVALID_ARG; }
    for (size_t i = 0; i < count; i++) {
        if (!buttons[i].capabilities.has_interrupt) { return ESP_ERR_NOT_SUPPORTED; }
    }
    if (ink_gesture_init(&s_gesture_state, &s_default_gesture_config) != INK_GESTURE_OK) { return ESP_ERR_INVALID_ARG; }

    esp_err_t error = create_queue_timer_task();
    if (error == ESP_OK) { error = connect_buttons(buttons); }
    if (error == ESP_OK) {
        error = start_console();
        if (error != ESP_OK) { ESP_LOGE(TAG, "console: %s", esp_err_to_name(error)); }
    }
    return error;
}
