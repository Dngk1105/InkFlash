#pragma once

#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_err.h"

/*
 * Nút bấm. Giao diện với logic là event INK_EVT_BUTTON trên bus (đã chống dội), không có hàm HAL.
 * Nút nối kiểu nào (kéo lên hay kéo xuống) khai báo ở đây, logic không cần biết.
 */
typedef struct {
    gpio_num_t pin_a;
    gpio_num_t pin_b;
    bool active_low; /* true: bấm = mức 0 (nối xuống GND, kéo lên nội) */
} drv_buttons_cfg_t;

esp_err_t drv_buttons_register(const drv_buttons_cfg_t *cfg);

/* Chẩn đoán: in mức điện thật của từng chân, số lần ngắt và số event đã phát. Dùng cho lệnh `btn` ở console. */
void drv_buttons_dump(void);
