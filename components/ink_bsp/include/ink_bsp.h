/* Thiết kế: docs/design/hardware-variants.md */
#pragma once

/* Bảng chân và driver của board. Tầng trên chỉ nhận device qua interface
 * ink_hal. */
#include "esp_err.h"
#include "ink_hal.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Khởi tạo driver theo bảng chân.
 * Gọi từ: app_main, một lần, trước mọi hàm ink_bsp khác.
 * @return lỗi của driver
 */
esp_err_t ink_bsp_init(void);

/**
 * Nút của board; index 0 là A, 1 là B.
 * @return ESP_ERR_INVALID_ARG nếu nullptr; ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_buttons(const ink_hal_button_device_t **out_devices, size_t *out_count);

/**
 * Màn của board.
 * @return ESP_ERR_INVALID_ARG nếu nullptr; ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_display(const ink_hal_display_device_t **out_device);
