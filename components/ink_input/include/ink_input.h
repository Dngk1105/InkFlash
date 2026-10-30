/* Thiết kế: docs/design/input/input.md */
#pragma once

/*
 * Vỏ của input: ngắt nút, đóng dấu giờ, timer theo deadline của ink_gesture, đọc lại chân, lệnh console `gesture`.
 * Không có task riêng: ISR, timer và console chỉ tạo message rồi đẩy qua hàm post của system; system gọi
 * ink_input_handle() trên task main. Chỉ task main đọc và sửa trạng thái của khối này.
 */
#include "esp_err.h"
#include "ink_gesture.h"
#include "ink_hal.h"
#include <stddef.h>
#include <stdint.h>

typedef enum : uint8_t {
    INK_INPUT_MESSAGE_BUTTON_EDGE = 0,
    INK_INPUT_MESSAGE_TIMER_EXPIRED,
    INK_INPUT_MESSAGE_SHOW_CONFIG,
    INK_INPUT_MESSAGE_SET_CONFIG,
    INK_INPUT_MESSAGE_SET_WAIT_MASK,
} ink_input_message_type_t;

typedef struct {
    ink_input_message_type_t type;

    union {
        ink_gesture_event_t button_edge;

        struct {
            uint8_t  field_index; /* index trong bảng trường config của khối */
            uint32_t value_ms;
        } set_config;

        struct {
            ink_gesture_button_t button;
            uint8_t              wait_mask; /* bit INK_GESTURE_WAIT_* */
        } set_wait_mask;
    };
} ink_input_message_t;

/* Đẩy message vào hàng đợi task main, không chờ. Bản _from_isr nằm trong IRAM, không log. */
typedef void (*ink_input_post_t)(const ink_input_message_t *message);
typedef void (*ink_input_post_from_isr_t)(const ink_input_message_t *message);

/*
 * Khởi tạo gesture, tạo timer, bật ngắt nút. Gọi một lần lúc khởi động, sau khi hàng đợi của system đã sẵn sàng.
 * `buttons`: index 0 là A, 1 là B; mảng phải sống suốt chương trình (khối đọc lại chân khi mất edge).
 * Trả ESP_ERR_INVALID_ARG nếu số nút khác 2 hoặc thiếu hàm post, ESP_ERR_NOT_SUPPORTED nếu nút không có ngắt,
 * ESP_ERR_INVALID_STATE nếu đã chạy, hoặc lỗi tạo timer, lỗi bật ngắt.
 */
esp_err_t ink_input_start(const ink_hal_button_device_t *buttons, size_t count, ink_input_post_t post,
                          ink_input_post_from_isr_t post_from_isr);

/* Xử lý một message của khối trên task main. Gesture vừa phát ghi vào `out_gestures` (đã log). */
void ink_input_handle(const ink_input_message_t *message, ink_gesture_output_t *out_gestures);

/*
 * Task main gọi khi hàng đợi rỗng. `messages_dropped`: có message rơi vì hàng đợi đầy kể từ lần gọi trước.
 * Đọc lại chân nếu có thể đã mất edge; gesture phát ra từ edge bù ghi vào `out_gestures`.
 */
void ink_input_on_queue_empty(bool messages_dropped, ink_gesture_output_t *out_gestures);

/*
 * Thêm lệnh console `gesture show | set <field> <ms> | enable <a|b> <mask>`. Gọi sau ink_input_start() và
 * ink_platform_console_init(). Trả ESP_ERR_INVALID_STATE nếu chưa start, hoặc lỗi của ink_platform_console_add().
 */
esp_err_t ink_input_add_console_command(void);
