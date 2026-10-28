/* Thiết kế: docs/design/display-pipeline.md */
#pragma once

/*
 * Driver màn WeAct 3,7" (controller UC8253, 240 × 416), làm theo interface ink_hal_display.
 * BSP khởi tạo bus SPI và truyền chân qua config. Bộ nhớ do bên gọi cấp, phải nằm trong RAM nội vì DMA đọc nó.
 */
#include <stdbool.h>
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

/* Biến thể partial refresh, để đo và so sánh. */
typedef struct {
    bool    pt_scan;           /* true: quét gate cả ngoài window (mặc định của PTL); false: chỉ trong window */
    bool    force_temperature; /* true: ép nhiệt độ `temperature`; false: cảm biến trong controller */
    uint8_t temperature;       /* mã °C cho lệnh TSSET; GxEPD2 dùng 110 */
    bool    keep_power_on;     /* true: không POF sau partial, lần partial sau bỏ được PON (như GxEPD2) */
} ink_drv_uc8253_partial_options_t;

typedef struct {
    ink_drv_uc8253_config_t          config;
    ink_drv_uc8253_partial_options_t partial_options;
    bool                             power_on; /* nguồn analog đang bật (sau PON, chưa POF) */
    spi_device_handle_t              spi;
    SemaphoreHandle_t                busy_ready; /* ISR give khi BUSY lên cao */
    StaticSemaphore_t                busy_ready_storage;
    uint8_t                          window_buffer[INK_DRV_UC8253_FRAME_BYTES] __attribute__((aligned(4)));
} ink_drv_uc8253_t;

/*
 * Reset controller, gửi cấu hình panel và điền `out_device`. Không đổi ảnh trên màn. Gọi một lần, từ task.
 * Trả ESP_ERR_INVALID_ARG nếu có con trỏ NULL, ESP_ERR_TIMEOUT nếu BUSY không lên, hoặc lỗi GPIO, SPI.
 */
esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config,
                              ink_hal_display_device_t *out_device);

/* Đổi biến thể partial cho các lần vẽ sau. Chỉ task màn gọi, giữa hai lần vẽ. Mặc định cả hai true. */
void ink_drv_uc8253_set_partial_options(ink_drv_uc8253_t *driver, const ink_drv_uc8253_partial_options_t *options);
