/* Thiết kế: docs/design/hardware-variants.md */
#pragma once

/*
 * Mẫu driver: một chân GPIO đầu vào, làm theo interface ink_hal_example.
 * Driver không tự biết chân: BSP truyền chân qua config lúc khởi tạo.
 * Bộ nhớ của driver do bên gọi cấp (struct ink_drv_example_t), driver không
 * malloc.
 */
#include "driver/gpio.h"
#include "esp_err.h"
#include "ink_hal_example.h"
#include <stdbool.h>

typedef struct {
  gpio_num_t pin;
  bool active_low; /* true: chân ở mức 0 nghĩa là đang tác động (ví dụ nút nối
                      GND) */
} ink_drv_example_config_t;

typedef struct {
  ink_drv_example_config_t config;
} ink_drv_example_t;

/*
 * Cấu hình chân và điền `out_device` để tầng trên dùng qua HAL.
 * Trả ESP_ERR_INVALID_ARG nếu có con trỏ NULL, hoặc lỗi của gpio_config().
 */
esp_err_t ink_drv_example_init(ink_drv_example_t *driver,
                               const ink_drv_example_config_t *config,
                               ink_hal_example_device_t *out_device);
