#pragma once

#include "drv_imu.h"
#include "ink_hal_imu.h"

/*
 * Khuôn nội bộ của một chip IMU (không phải API công khai). Mỗi imu_<chip>.c xuất một biến const drv_imu_chip_t,
 * drv_imu.c chọn biến theo Kconfig. Hàm nào chip không cần thì để NULL.
 */
typedef struct {
    const ink_imu_ops_t *ops;                 /* đi vào HAL */
    esp_err_t (*init)(const drv_imu_cfg_t *cfg); /* dò chip, cấu hình; gọi lại được (dùng cho recover) */
    esp_err_t (*self_test)(void);
    esp_err_t (*suspend)(void);
    esp_err_t (*resume)(void);
} drv_imu_chip_t;
