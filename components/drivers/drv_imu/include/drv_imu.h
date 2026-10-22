#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

/* Cách nối IMU trên board. BSP điền, driver chỉ đọc; driver không tự đọc macro chân. */
typedef struct {
    gpio_num_t sda;
    gpio_num_t scl;
    gpio_num_t int1;     /* GPIO_NUM_NC nếu không nối */
    uint32_t i2c_hz;     /* tần số bus I2C */
    uint8_t i2c_addr;    /* 0 = dùng địa chỉ mặc định của chip */
    int8_t mount[3][3];  /* chip → hệ trục thiết bị, xem ink_hal_imu.h */
} drv_imu_cfg_t;

/* Đăng ký IMU của board vào device registry. Chọn chip qua menuconfig (INK_IMU_DRIVER). */
esp_err_t drv_imu_register(const drv_imu_cfg_t *cfg);

#if CONFIG_INK_IMU_FAKE
/* Chỉ cho driver giả lập: đặt giá trị gia tốc để thử logic mà không cần phần cứng. */
void imu_fake_set(int16_t x_mg, int16_t y_mg, int16_t z_mg);
#endif
