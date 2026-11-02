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

/** Bảng sóng LUT chỉnh tay cho màn của board. */
typedef struct {
    uint8_t  frames;        /**< số khung kéo; 0 = chép nguyên bảng của panel */
    uint16_t frame_rate_hz; /**< 0 = tần số của panel */
    uint8_t  hold_frames;   /**< xung giữ cho điểm không đổi */
    uint8_t  scans;         /**< số lần quét mỗi lần vẽ, ≥ 1 */
    bool     keep_power_on; /**< giữ nguồn analog giữa các lần vẽ */
} ink_bsp_display_tune_t;

/**
 * Gán bảng sóng chỉnh tay cho slot, có tác dụng từ lần vẽ sau. Chỉ firmware test dùng.
 * Gọi từ: task khác task màn.
 * @return ESP_ERR_INVALID_ARG nếu nullptr hoặc thông số bị driver từ chối (ngoài khoảng, lệch điện tích);
 *         ESP_ERR_INVALID_STATE nếu chưa init
 */
esp_err_t ink_bsp_display_tune(ink_hal_display_slot_t slot, const ink_bsp_display_tune_t *tune);
