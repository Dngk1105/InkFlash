#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

/* Cách nối mic I2S trên board. BSP điền. Mic và loa có thể dùng chung BCLK/WS. */
typedef struct {
    gpio_num_t bclk, ws, din;
} drv_mic_cfg_t;

typedef struct {
    gpio_num_t bclk, ws, dout;
    gpio_num_t sd; /* chân bật/tắt amp (SD_MODE của MAX98357A) */
} drv_speaker_cfg_t;

esp_err_t drv_mic_register(const drv_mic_cfg_t *cfg);
esp_err_t drv_speaker_register(const drv_speaker_cfg_t *cfg);
