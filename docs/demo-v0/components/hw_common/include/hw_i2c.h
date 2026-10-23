#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

/*
 * Bus I2C dùng chung. Nhiều driver (IMU, chip sạc) cùng một cặp chân SDA/SCL thì phải dùng chung một bus.
 * Lần gọi đầu tạo bus; các lần sau trả về bus đã có (chân phải trùng, nếu không báo lỗi).
 * Chỉ driver được include file này. Service không biết gì về I2C.
 */
esp_err_t hw_i2c_get_bus(gpio_num_t sda, gpio_num_t scl, i2c_master_bus_handle_t *out);
