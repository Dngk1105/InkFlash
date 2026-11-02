/* Thiết kế: docs/design/tasks.md */
#pragma once

/*
 * Task main: giữ trạng thái, nhận message từ hàng đợi rồi giao cho khối chủ. ISR, callback timer và lệnh console
 * của các khối chỉ đẩy message vào hàng đợi này (docs/design/tasks.md).
 */
#include "esp_err.h"
#include "ink_hal.h"
#include <stddef.h>

/** Thêm lệnh console riêng của board (ví dụ `epd`), gọi giữa lúc khởi tạo và lúc bật console. */
typedef esp_err_t (*ink_system_console_hook_t)(void);

typedef struct {
    const ink_hal_button_device_t  *buttons; /**< index 0 là A, 1 là B; mảng sống suốt chương trình (input đọc lại chân khi mất edge) */
    size_t                          button_count;
    const ink_hal_display_device_t *display;
    ink_system_console_hook_t       add_board_commands; /**< có thể NULL */
} ink_system_config_t;

/**
 * Tạo hàng đợi và task main, khởi động input (bật ngắt nút) và display, bật console.
 * Gọi từ: app_main, một lần, sau ink_bsp_init().
 * @return ESP_ERR_INVALID_ARG nếu config hoặc display nullptr hay số nút khác 2; ESP_ERR_NOT_SUPPORTED nếu nút
 *         không có ngắt; ESP_ERR_INVALID_STATE nếu đã chạy; ESP_ERR_NO_MEM nếu không tạo được hàng đợi, task hoặc khung
 */
esp_err_t ink_system_start(const ink_system_config_t *config);
