/* Thiết kế: docs/design/input.md */
#pragma once

/* Interface nút: một device cho một nút. Driver không đóng dấu thời gian, bên
 * nhận callback đóng dấu. */
#include "esp_err.h"
#include <stdbool.h>

/* Chạy trong ISR mỗi khi chân đổi mức: phải nằm trong IRAM, không log, không
 * chặn. */
typedef void (*ink_hal_button_callback_t)(void *user_data, bool pressed);

/* Bảng khả năng: driver khai báo nó làm được gì. */
typedef struct {
    bool has_interrupt; /* báo edge bằng ngắt qua set_callback */
} ink_hal_button_capabilities_t;

/* Bảng hàm. `context` là dữ liệu riêng của driver. */
typedef struct {
    esp_err_t (*read)(void *context, bool *out_pressed);
    /* Đăng ký callback rồi bật ngắt; gọi một lần, từ task. */
    esp_err_t (*set_callback)(void *context, ink_hal_button_callback_t callback, void *user_data);
} ink_hal_button_operations_t;

typedef struct {
    const ink_hal_button_operations_t *operations;
    void                              *context;
    ink_hal_button_capabilities_t      capabilities;
} ink_hal_button_device_t;
