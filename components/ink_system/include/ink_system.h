/* Thiết kế: docs/design/input.md */
#pragma once

/*
 * Task main: giữ trạng thái, nhận sự kiện từ hàng đợi. ISR, callback timer và lệnh console chỉ đẩy message vào
 * hàng đợi này (docs/design/tasks.md).
 */
#include <stddef.h>
#include "esp_err.h"
#include "ink_hal.h"

/*
 * Tạo hàng đợi, timer, task main; bật ngắt nút và console. Gọi một lần lúc khởi động, sau ink_bsp_init().
 * `buttons`: index 0 là A, 1 là B.
 * Trả ESP_ERR_INVALID_ARG nếu số nút khác 2, ESP_ERR_NOT_SUPPORTED nếu nút không có ngắt,
 * ESP_ERR_INVALID_STATE nếu đã chạy, ESP_ERR_NO_MEM nếu không tạo được hàng đợi hoặc task.
 */
esp_err_t ink_system_start(const ink_hal_button_device_t *buttons, size_t count);
