/* Thiết kế: docs/design/display-pipeline.md */
#pragma once

/*
 * Driver màn WeAct 3,7" (controller UC8253, 240 × 416), làm theo interface ink_hal_display.
 * BSP khởi tạo bus SPI và truyền chân qua config. Bộ nhớ do bên gọi cấp, phải nằm trong RAM nội vì DMA đọc nó.
 */
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ink_hal_display.h"

#define INK_DRV_UC8253_WIDTH       240
#define INK_DRV_UC8253_HEIGHT      416
#define INK_DRV_UC8253_FRAME_BYTES (INK_DRV_UC8253_WIDTH / 8 * INK_DRV_UC8253_HEIGHT)

typedef struct {
    spi_host_device_t host; /* bus đã khởi tạo, max_transfer_sz ≥ INK_DRV_UC8253_FRAME_BYTES */
    gpio_num_t        cs;
    gpio_num_t        dc;
    gpio_num_t        reset;
    gpio_num_t        busy;
} ink_drv_uc8253_config_t;

typedef struct {
    ink_drv_uc8253_config_t config;
    spi_device_handle_t     spi;
    SemaphoreHandle_t       busy_ready; /* ISR give khi BUSY lên cao */
    StaticSemaphore_t       busy_ready_storage;
    uint8_t                 window_buffer[INK_DRV_UC8253_FRAME_BYTES] __attribute__((aligned(4)));
} ink_drv_uc8253_t;

/*
 * Reset controller, gửi cấu hình panel và điền `out_device`. Không đổi ảnh trên màn. Gọi một lần, từ task.
 * Trả ESP_ERR_INVALID_ARG nếu có con trỏ NULL, ESP_ERR_TIMEOUT nếu BUSY không lên, hoặc lỗi GPIO, SPI.
 */
esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config,
                              ink_hal_display_device_t *out_device);
