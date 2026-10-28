/* Thiết kế: docs/design/hardware-variants.md */
#pragma once

/* Bảng chân và driver của board. Tầng trên chỉ nhận device qua interface
 * ink_hal. */
#include "esp_err.h"
#include "ink_hal.h"
#include <stddef.h>

/* Khởi tạo driver theo bảng chân. Gọi một lần lúc khởi động, trước mọi hàm
 * ink_bsp khác. Trả lỗi của driver. */
esp_err_t ink_bsp_init(void);

/*
 * Nút của board; index 0 là A, 1 là B.
 * Trả ESP_ERR_INVALID_ARG nếu NULL, ESP_ERR_INVALID_STATE nếu chưa init.
 */
esp_err_t ink_bsp_buttons(const ink_hal_button_device_t **out_devices, size_t *out_count);

/* Màn của board. Trả ESP_ERR_INVALID_ARG nếu NULL, ESP_ERR_INVALID_STATE nếu
 * chưa init. */
esp_err_t ink_bsp_display(const ink_hal_display_device_t **out_device);
