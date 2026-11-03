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

/**
 * Đổi byte 2 của lệnh PSR màn (VCOM, VGL sau khi vẽ và tắt nguồn), có tác dụng từ lần vẽ hoặc tắt nguồn sau.
 * Chỉ firmware test dùng.
 * Gọi từ: task bất kỳ.
 * @return ESP_ERR_INVALID_ARG nếu `value` có bit D7–D5; ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_display_set_psr_second(uint8_t value);

/**
 * Byte 2 của PSR màn đã đặt gần nhất.
 * Gọi từ: task bất kỳ, sau ink_bsp_init.
 */
uint8_t ink_bsp_display_psr_second(void);

/**
 * Thay VCOM_DC của mọi profile màn bằng `value` (byte lệnh 0x82), có tác dụng từ lần vẽ sau; 0xFF = theo từng profile.
 * Chỉ firmware test dùng.
 * Gọi từ: task bất kỳ.
 * @return ESP_ERR_INVALID_ARG nếu `value` lớn hơn 0x7E mà không phải 0xFF; ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_display_set_vcom_override(uint8_t value);

/**
 * VCOM_DC thay thế đã đặt gần nhất; 0xFF = theo từng profile.
 * Gọi từ: task bất kỳ, sau ink_bsp_init.
 */
uint8_t ink_bsp_display_vcom_override(void);

/**
 * Đặt số khung từ lúc tắt source tới lúc tắt gate khi tắt nguồn màn (1–4), có tác dụng từ lần vẽ hoặc tắt nguồn sau.
 * Chỉ firmware test dùng.
 * Gọi từ: task bất kỳ.
 * @return ESP_ERR_INVALID_ARG nếu `frames` ngoài 1–4; ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_display_set_power_off_frames(uint8_t frames);

/**
 * Số khung tắt nguồn đã đặt gần nhất.
 * Gọi từ: task bất kỳ, sau ink_bsp_init.
 */
uint8_t ink_bsp_display_power_off_frames(void);
