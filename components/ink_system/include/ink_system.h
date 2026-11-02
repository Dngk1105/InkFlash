/* Thiết kế: docs/design/tasks.md */
#pragma once

/*
 * Task main: giữ trạng thái, nhận message từ hàng đợi rồi giao cho khối chủ. ISR và callback timer của các khối
 * chỉ đẩy message vào hàng đợi này (docs/design/tasks.md).
 */
#include "esp_err.h"
#include "ink_display.h"
#include "ink_hal.h"
#include <stddef.h>

typedef struct {
    const ink_hal_button_device_t  *buttons; /**< index 0 là A, 1 là B; mảng sống suốt chương trình (input đọc lại chân khi mất edge) */
    size_t                          button_count;
    const ink_hal_display_device_t *display;
    const ink_display_options_t    *display_options; /**< cài đặt màn đã lưu; nullptr = mặc định */
} ink_system_config_t;

/**
 * Tạo hàng đợi và task main, khởi động input (bật ngắt nút) và display.
 * Gọi từ: app_main, một lần, sau ink_bsp_init().
 * @return ESP_ERR_INVALID_ARG nếu config hoặc display nullptr, số nút khác 2 hay cài đặt màn sai; ESP_ERR_NOT_SUPPORTED nếu nút
 *         không có ngắt; ESP_ERR_INVALID_STATE nếu đã chạy; ESP_ERR_NO_MEM nếu không tạo được hàng đợi, task hoặc khung
 */
esp_err_t ink_system_start(const ink_system_config_t *config);
