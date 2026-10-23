#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL đèn trạng thái RGB. Màu logic 0–255 mỗi kênh. Giới hạn độ sáng tối đa do driver/board quyết định
 * (LED WS2812 rất chói), logic không cần biết.
 */
typedef struct {
    esp_err_t (*set_rgb)(void *ctx, uint8_t r, uint8_t g, uint8_t b);
} ink_led_ops_t;

esp_err_t ink_led_attach(const ink_led_ops_t *ops, void *ctx);
bool ink_led_present(void);
/* Không có LED thì trả ESP_ERR_NOT_FOUND; logic không cần xử lý, chỉ bỏ qua. */
esp_err_t ink_led_set(uint8_t r, uint8_t g, uint8_t b);
