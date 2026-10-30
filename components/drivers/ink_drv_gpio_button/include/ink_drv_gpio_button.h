/* Thiết kế: docs/design/input.md */
#pragma once

/*
 * Driver nút nối thẳng vào một chân GPIO, làm theo interface ink_hal_button.
 * Báo edge bằng ngắt. BSP truyền chân qua config. Bộ nhớ do bên gọi cấp, phải
 * nằm trong RAM nội vì ISR đọc nó.
 */
#include "driver/gpio.h"
#include "esp_err.h"
#include "ink_hal_button.h"
#include <stdbool.h>

typedef struct {
    gpio_num_t pin;
    bool       active_low; /* true: mức 0 là đang bấm (nút nối GND) */
    bool       pull_up;    /* bật kéo lên trong */
} ink_drv_gpio_button_config_t;

typedef struct {
    ink_drv_gpio_button_config_t config;
    ink_hal_button_callback_t    callback;
    void                        *callback_user_data;
} ink_drv_gpio_button_t;

/*
 * Cấu hình chân (chưa bật ngắt) và điền `out_device`. Bên gọi cài gpio_install_isr_service() trước khi gọi set_callback.
 * Trả ESP_ERR_INVALID_ARG nếu có con trỏ NULL, hoặc lỗi của gpio_config().
 */
esp_err_t ink_drv_gpio_button_init(ink_drv_gpio_button_t *driver, const ink_drv_gpio_button_config_t *config,
                                   ink_hal_button_device_t *out_device);
