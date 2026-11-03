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

/**
 * Nạp gói profile `.inkw` (tools/profile_gen.py --blob) làm profile tạm của màn, có tác dụng từ lần vẽ sau.
 * Chỉ firmware test dùng.
 * Gọi từ: task bất kỳ.
 * @return ESP_ERR_INVALID_ARG nếu nullptr hoặc gói sai (giữ profile tạm cũ); ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_display_load_profile(const uint8_t *blob, size_t length);

/**
 * Đọc OTP của controller màn vào `out` (bank 0 tới hết bảng sóng). Chỉ firmware test dùng.
 * Gọi từ: task đang gọi vẽ màn, giữa hai lần vẽ.
 * @param out_length số byte đã đọc
 * @return ESP_ERR_INVALID_ARG nếu nullptr hoặc `capacity` nhỏ hơn bank; ESP_ERR_INVALID_STATE nếu chưa init; lỗi SPI
 */
esp_err_t ink_bsp_display_read_otp(uint8_t *out, size_t capacity, size_t *out_length);
