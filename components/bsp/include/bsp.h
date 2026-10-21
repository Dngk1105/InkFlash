#pragma once

#include "esp_err.h"
#include "bsp_pins.h"

/*
 * Đăng ký mọi thiết bị của board vào device registry, kèm cấu hình chân lấy từ file board.
 * Chưa init: init do ink_device_init_all(). Trả lỗi nếu bảng chân sai (trùng chân).
 */
esp_err_t bsp_register_devices(void);
