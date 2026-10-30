/* Thiết kế: docs/design/tasks.md */
#include "ink_system.h"

#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_input.h"
#include "ink_platform.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "ink_system";

/* ---- Message vào task main ---- */
/* ISR, timer và console của các khối chỉ tạo message; mọi việc làm trên task main (docs/design/tasks.md). */

typedef enum : uint8_t {
    MESSAGE_SOURCE_INPUT = 0,
} message_source_t;

typedef struct {
    message_source_t source;

    union {
        ink_input_message_t input;
    };
} message_t;

/* ---- Hàng đợi task main ---- */

#define QUEUE_LENGTH 32 /* đủ cho một chùm edge rung của cả hai nút */

static QueueHandle_t s_queue;
static atomic_uint   s_dropped_count; /* message rơi vì hàng đợi đầy; ISR và task phụ cộng, task main đọc */

/* Gọi từ task phụ (timer, console), không chờ. */
static void post_message(const message_t *message) {
    if (xQueueSend(s_queue, message, 0) != pdTRUE) { atomic_fetch_add(&s_dropped_count, 1); }
}

static void IRAM_ATTR post_message_from_isr(const message_t *message) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    if (xQueueSendFromISR(s_queue, message, &higher_priority_task_woken) != pdTRUE) { atomic_fetch_add(&s_dropped_count, 1); }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/* ---- Hàm post giao cho từng khối ---- */

static void post_input(const ink_input_message_t *input) {
    const message_t message = {.source = MESSAGE_SOURCE_INPUT, .input = *input};
    post_message(&message);
}

static void IRAM_ATTR post_input_from_isr(const ink_input_message_t *input) {
    const message_t message = {.source = MESSAGE_SOURCE_INPUT, .input = *input};
    post_message_from_isr(&message);
}

/* ---- Task main ---- */

#define TASK_STACK_BYTES 4096 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY    5

static void task_main(void *user_data) {
    (void)user_data;
    message_t            message;
    ink_gesture_output_t gestures;
    bool                 messages_dropped = false; /* có message rơi, chưa báo cho input */
    for (;;) {
        if (xQueueReceive(s_queue, &message, portMAX_DELAY) != pdTRUE) { continue; }
        const unsigned dropped_count = atomic_exchange(&s_dropped_count, 0);
        if (dropped_count) {
            ESP_LOGW(TAG, "hàng đợi đầy, rơi %u message", dropped_count);
            messages_dropped = true;
        }
        switch (message.source) {
            case MESSAGE_SOURCE_INPUT:
                ink_input_handle(&message.input, &gestures);
                break;
        }
        if (uxQueueMessagesWaiting(s_queue) == 0) {
            ink_input_on_queue_empty(messages_dropped, &gestures);
            messages_dropped = false;
        }
    }
}

/* ---- Khởi động ---- */

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

static esp_err_t start_console(void) {
    esp_err_t error = ink_platform_console_init("ink> ");
    if (error == ESP_OK) { error = ink_input_add_console_command(); }
    if (error == ESP_OK) { error = ink_platform_console_start(); }
    return error;
}

esp_err_t ink_system_start(const ink_hal_button_device_t *buttons, size_t count) {
    if (s_queue != nullptr) { return ESP_ERR_INVALID_STATE; }
    esp_err_t error = create_queue_task();
    if (error == ESP_OK) { error = ink_input_start(buttons, count, post_input, post_input_from_isr); }
    if (error == ESP_OK) {
        error = start_console();
        if (error != ESP_OK) { ESP_LOGE(TAG, "console: %s", esp_err_to_name(error)); }
    }
    return error;
}
