#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "ink_hal_power.h"

/* Cách nối khối nguồn trên board. BSP điền. */
typedef struct {
    gpio_num_t i2c_sda, i2c_scl;      /* chip sạc */
    uint32_t i2c_hz;
    gpio_num_t chg_int;               /* ngắt từ chip sạc */
    gpio_num_t bat_adc;               /* đo pin qua ADC (nếu không đo qua chip sạc) */
    gpio_num_t rail_en[INK_RAIL_COUNT]; /* chân bật nguồn từng khối phụ */
    uint64_t wake_mask;               /* bit n = GPIO n đánh thức khỏi deep sleep (nút A/B) */
} drv_power_cfg_t;

esp_err_t drv_power_register(const drv_power_cfg_t *cfg);
