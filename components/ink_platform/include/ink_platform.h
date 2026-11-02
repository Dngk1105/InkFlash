/* Thiết kế: docs/design/input/input.md */
#pragma once

/* Giờ, timer một lần. Không có task riêng: callback timer chạy trên task
 * esp_timer, là task phụ, chỉ được đẩy message vào hàng đợi task main
 * (docs/design/tasks.md). */
#include "esp_err.h"
#include <stdint.h>

/* ---- Giờ ---- */

/**
 * ms từ lúc khởi động, quay vòng sau ~49 ngày.
 * Gọi từ: mọi nơi, kể cả ISR (nằm trong IRAM).
 */
uint32_t ink_platform_now_ms(void);

/* ---- Timer một lần ---- */

typedef struct ink_platform_timer *ink_platform_timer_t;

typedef void (*ink_platform_timer_callback_t)(void *user_data);

/**
 * Tạo timer, chưa chạy.
 * @param name phải sống suốt đời timer
 * @return ESP_ERR_INVALID_ARG nếu callback hoặc out_timer là nullptr; ESP_ERR_NO_MEM nếu hết bộ nhớ
 */
esp_err_t ink_platform_timer_create(ink_platform_timer_callback_t callback, void *user_data, const char *name,
                                    ink_platform_timer_t *out_timer);

/**
 * Hẹn callback sau delay_ms. Timer đang chạy thì huỷ hẹn cũ.
 * @return ESP_ERR_INVALID_ARG nếu timer nullptr
 */
esp_err_t ink_platform_timer_start(ink_platform_timer_t timer, uint32_t delay_ms);

/**
 * Huỷ hẹn; timer không chạy thì không làm gì.
 * @return ESP_ERR_INVALID_ARG nếu timer nullptr
 */
esp_err_t ink_platform_timer_stop(ink_platform_timer_t timer);
