/* Thiết kế: docs/design/tasks.md */
#pragma once

/*
 * Task main: giữ trạng thái, nhận message từ hàng đợi rồi giao cho khối chủ. ISR, callback timer và lệnh console
 * của các khối chỉ đẩy message vào hàng đợi này (docs/design/tasks.md).
 */
#include "esp_err.h"
#include "ink_hal.h"
#include <stddef.h>

/*
 * Tạo hàng đợi và task main, khởi động input (bật ngắt nút), bật console. Gọi một lần lúc khởi động, sau
 * ink_bsp_init(). `buttons`: index 0 là A, 1 là B; mảng phải sống suốt chương trình (input đọc lại chân khi mất
 * edge). Trả ESP_ERR_INVALID_ARG nếu số nút khác 2, ESP_ERR_NOT_SUPPORTED nếu nút không có ngắt,
 * ESP_ERR_INVALID_STATE nếu đã chạy, ESP_ERR_NO_MEM nếu không tạo được hàng đợi hoặc task.
 */
esp_err_t ink_system_start(const ink_hal_button_device_t *buttons, size_t count);
