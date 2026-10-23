#pragma once

#include "esp_err.h"

/*
 * Composition root: nơi DUY NHẤT biết board đang dùng chip nào và nối chân nào.
 * bsp_init() kiểm tra bảng chân rồi đăng ký driver của board vào device registry (ink_device_register).
 * Chưa init driver: việc đó do ink_device_init_all() làm sau.
 */
esp_err_t bsp_init(void);

const char *bsp_board_name(void);
