#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

/* Cách nối màn trên board. BSP điền, driver chỉ đọc. GPIO_NUM_NC = chưa nối. */
typedef struct {
    gpio_num_t sck, mosi, cs, dc, rst, busy;
    /*
     * Xoay màn: 0–3 theo quy ước Adafruit_GFX (1 = xoay 90° để màn dọc 240×416 thành ngang 416×240).
     * Driver tự đổi tọa độ; logic chỉ thấy kích thước sau khi xoay qua ink_display_get_info().
     */
    uint8_t rotation;
} drv_display_cfg_t;

esp_err_t drv_display_register(const drv_display_cfg_t *cfg);
