#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* HAL đèn trạng thái RGB, màu logic 0–255. Trần độ sáng do driver/board quyết định. */
typedef struct {
    esp_err_t (*set_rgb)(void *ctx, uint8_t r, uint8_t g, uint8_t b);
} ink_led_ops_t;

esp_err_t ink_led_attach(const ink_led_ops_t *ops, void *ctx);
bool ink_led_present(void);
esp_err_t ink_led_set(uint8_t r, uint8_t g, uint8_t b); /* không có LED: ESP_ERR_NOT_FOUND, bỏ qua được */
