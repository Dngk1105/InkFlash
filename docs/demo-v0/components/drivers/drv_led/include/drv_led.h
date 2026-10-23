#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

/* Cách nối LED trên board. BSP điền. */
typedef struct {
    gpio_num_t pin;      /* GPIO_NUM_NC = board không có LED */
    uint8_t brightness;  /* trần độ sáng 0–255; WS2812 rất chói nên demo dùng 50 */
} drv_led_cfg_t;

esp_err_t drv_led_register(const drv_led_cfg_t *cfg);
