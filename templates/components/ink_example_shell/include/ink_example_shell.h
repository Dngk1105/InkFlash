/* Thiết kế: docs/design/code-structure.md */
#pragma once

/*
 * Mẫu khối vỏ có task và hàng đợi. Khối khác gửi message vào hàng đợi; chỉ task
 * của khối đọc và sửa trạng thái. Phần quyết định nên nằm ở một khối C thuần;
 * khối vỏ chỉ nhận message, gọi khối C thuần, rồi làm theo kết quả.
 */
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t value;
} ink_example_shell_message_t;

/*
 * Tạo hàng đợi và task. Gọi một lần lúc khởi động, từ task main.
 * Trả ESP_ERR_INVALID_STATE nếu đã chạy, ESP_ERR_NO_MEM nếu không tạo được hàng
 * đợi hoặc task.
 */
esp_err_t ink_example_shell_start(void);

/*
 * Gửi message từ task, không chờ.
 * Trả ESP_ERR_INVALID_STATE nếu chưa start, ESP_ERR_TIMEOUT nếu hàng đợi đầy.
 */
esp_err_t ink_example_shell_post(const ink_example_shell_message_t *message);

/*
 * Gửi message từ ISR. Hàm nằm trong IRAM, không log, không cấp phát.
 * Trả false nếu hàng đợi đầy hoặc chưa start.
 */
bool ink_example_shell_post_from_isr(
    const ink_example_shell_message_t *message);
