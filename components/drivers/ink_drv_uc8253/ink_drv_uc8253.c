/* Thiết kế: docs/design/display-pipeline.md */

/* Lệnh và trình tự: docs/hardware/parts/weact-epaper-3.7/ (commands.md,
 * sequences.md), làm theo GxEPD2. */
#include "ink_drv_uc8253.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "ink_drv_uc8253";

#define SPI_CLOCK_HZ    (4 * 1000 * 1000)
#define ROW_BYTES       (INK_DRV_UC8253_WIDTH / 8)
#define X_ALIGN         8 /* PTL: HRST thấp 3 bit = 000, HRED = 111 */
#define TYPICAL_FULL_MS 2160
#define TYPICAL_PARTIAL_MS                               \
    425 /* khung chữ 80 × 40, đo trên BB0 (refresh.md) \
         */

#define CMD_PSR   0x00
#define CMD_POF   0x02
#define CMD_PON   0x04
#define CMD_DTM1  0x10 /* ảnh cũ */
#define CMD_DRF   0x12
#define CMD_DTM2  0x13 /* ảnh mới */
#define CMD_CDI   0x50
#define CMD_PTL   0x90
#define CMD_PTIN  0x91
#define CMD_PTOUT 0x92
#define CMD_CCSET 0xE0
#define CMD_TSSET 0xE5

#define BUSY_SHORT_TIMEOUT_MS   200
#define BUSY_REFRESH_TIMEOUT_MS 5000

/* ---- SPI ---- */

static esp_err_t write_bytes(ink_drv_uc8253_t *driver, bool is_data, const uint8_t *bytes, size_t length) {
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.dc, is_data), TAG, "dc");
    spi_transaction_t transaction = {.length = length * 8};
    if (length <= sizeof(transaction.tx_data)) {
        transaction.flags = SPI_TRANS_USE_TXDATA;
        memcpy(transaction.tx_data, bytes, length);
        return spi_device_polling_transmit(driver->spi, &transaction);
    }
    /* ảnh lớn: chờ bằng ngắt, không chiếm CPU */
    transaction.tx_buffer = bytes;
    return spi_device_transmit(driver->spi, &transaction);
}

static esp_err_t send_command(ink_drv_uc8253_t *driver, uint8_t command, const uint8_t *data, size_t length) {
    ESP_RETURN_ON_ERROR(write_bytes(driver, false, &command, 1), TAG, "cmd 0x%02x", command);
    if (length > 0) { ESP_RETURN_ON_ERROR(write_bytes(driver, true, data, length), TAG, "data 0x%02x", command); }
    return ESP_OK;
}

/* ---- BUSY ---- */
/* BUSY thấp = bận. Lệnh có cờ BUSY: xoá semaphore trước khi gửi, rồi chờ edge
 * lên. */

static void IRAM_ATTR on_busy_rise(void *user_data) {
    ink_drv_uc8253_t *driver                     = user_data;
    BaseType_t        higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(driver->busy_ready, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static esp_err_t wait_ready(ink_drv_uc8253_t *driver, uint32_t timeout_ms) {
    const int64_t deadline_us = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (gpio_get_level(driver->config.busy) == 0) {
        const int64_t left_us = deadline_us - esp_timer_get_time();
        ESP_RETURN_ON_FALSE(left_us > 0, ESP_ERR_TIMEOUT, TAG, "busy timeout");
        xSemaphoreTake(driver->busy_ready, pdMS_TO_TICKS(left_us / 1000 + 1));
    }
    return ESP_OK;
}

/* Gửi lệnh làm BUSY xuống thấp rồi ngủ tới khi BUSY lên lại. */
static esp_err_t run_busy_command(ink_drv_uc8253_t *driver, uint8_t command, const uint8_t *data, size_t length, uint32_t timeout_ms) {
    xSemaphoreTake(driver->busy_ready, 0);
    ESP_RETURN_ON_ERROR(send_command(driver, command, data, length), TAG, "busy cmd");
    /* BUSY có thể chưa kịp xuống ngay sau CS lên: chờ edge lên trước, rồi mới tin
     * mức chân */
    ESP_RETURN_ON_FALSE(xSemaphoreTake(driver->busy_ready, pdMS_TO_TICKS(timeout_ms)) == pdTRUE, ESP_ERR_TIMEOUT, TAG,
                        "busy timeout cmd 0x%02x", command);
    return wait_ready(driver, timeout_ms);
}

/* ---- Khởi tạo controller ---- */

static esp_err_t reset_controller(ink_drv_uc8253_t *driver) {
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.reset, 0), TAG, "reset low");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.reset, 1), TAG, "reset high");
    vTaskDelay(pdMS_TO_TICKS(10)); /* > 1 ms, lệnh gửi sớm hơn bị bỏ qua */
    ESP_RETURN_ON_ERROR(wait_ready(driver, BUSY_SHORT_TIMEOUT_MS), TAG, "after reset");

    /* PSR như GxEPD2: KW, bảng sóng OTP; 0x1E có RST_N = 0 nên là reset mềm */
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PSR, (const uint8_t[]){0x1E, 0x0D}, 2, BUSY_SHORT_TIMEOUT_MS), TAG, "soft reset");
    return send_command(driver, CMD_PSR, (const uint8_t[]){0x1F, 0x0D}, 2);
}

/* ---- Vẽ ---- */

/* Vào chế độ partial với window đã căn mép. Bên gọi gửi PTOUT khi xong. */
static esp_err_t enter_window(ink_drv_uc8253_t *driver, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1, bool pt_scan) {
    const uint8_t ptl[7] = {x0, x1, y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF, pt_scan ? 0x01 : 0x00};
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PTIN, nullptr, 0), TAG, "ptin");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PTL, ptl, sizeof(ptl)), TAG, "ptl");
    /* PTL có cờ BUSY (UC8253 tr.46) */
    return wait_ready(driver, BUSY_SHORT_TIMEOUT_MS);
}

/* Ghi window đã căn mép vào một vùng RAM ảnh. Dữ liệu window nằm sẵn trong
 * window_buffer. */
static esp_err_t write_window(ink_drv_uc8253_t *driver, uint8_t ram_command, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1) {
    const uint16_t row_bytes    = (x1 - x0 + 1) / 8;
    const size_t   window_bytes = (size_t)row_bytes * (y1 - y0 + 1);
    ESP_RETURN_ON_ERROR(enter_window(driver, x0, x1, y0, y1, true), TAG, "window");
    ESP_RETURN_ON_ERROR(send_command(driver, ram_command, driver->window_buffer, window_bytes), TAG, "ram");
    return send_command(driver, CMD_PTOUT, nullptr, 0);
}

static void copy_window(ink_drv_uc8253_t *driver, const uint8_t *frame, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1) {
    const uint16_t row_bytes = (x1 - x0 + 1) / 8;
    for (uint16_t y = y0; y <= y1; y++) {
        memcpy(&driver->window_buffer[(size_t)(y - y0) * row_bytes], &frame[(size_t)y * ROW_BYTES + x0 / 8], row_bytes);
    }
}

static uint32_t elapsed_us(int64_t *since_us) {
    const int64_t now_us = esp_timer_get_time();
    const int64_t result = now_us - *since_us;
    *since_us            = now_us;
    return (uint32_t)result;
}

/* ---- Bảng hàm ink_hal_display ---- */

/* Datasheet không nói PON khi đang bật có kéo BUSY xuống không;
 * run_busy_command chờ edge lên nên có thể đợi tới timeout. Chỉ gửi khi đang
 * tắt. */
static esp_err_t power_on(ink_drv_uc8253_t *driver) {
    if (driver->power_on) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PON, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pon");
    driver->power_on = true;
    return ESP_OK;
}

static esp_err_t power_off(ink_drv_uc8253_t *driver) {
    if (!driver->power_on) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_POF, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pof");
    driver->power_on = false;
    return ESP_OK;
}

/* Sóng nhanh của GxEPD2: ép nhiệt độ 90; CDI 0x97: bit 1 = trắng, viền làm mới
 * theo VBD 10. */
static esp_err_t refresh_full(ink_drv_uc8253_t *driver) {
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CCSET, (const uint8_t[]){0x02}, 1), TAG, "ccset");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_TSSET, (const uint8_t[]){0x5A}, 1), TAG, "tsset");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CDI, (const uint8_t[]){0x97}, 1), TAG, "cdi");
    ESP_RETURN_ON_ERROR(power_on(driver), TAG, "pon");
    return run_busy_command(driver, CMD_DRF, nullptr, 0, BUSY_REFRESH_TIMEOUT_MS);
}

/* Như GxEPD2: DRF trong chế độ partial; CDI 0xD7: bit 1 = trắng, viền nổi (VBD
 * 11). */
static esp_err_t refresh_partial(ink_drv_uc8253_t *driver, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1) {
    const ink_drv_uc8253_partial_options_t *options = &driver->partial_options;
    ESP_RETURN_ON_ERROR(enter_window(driver, x0, x1, y0, y1, options->pt_scan), TAG, "window");
    /* TSFIX còn từ lần vẽ trước: không ép thì phải xoá để controller đo nhiệt độ
     * thật */
    const uint8_t ccset = options->force_temperature ? 0x02 : 0x00;
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CCSET, &ccset, 1), TAG, "ccset");
    if (options->force_temperature) { ESP_RETURN_ON_ERROR(send_command(driver, CMD_TSSET, &options->temperature, 1), TAG, "tsset"); }
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CDI, (const uint8_t[]){0xD7}, 1), TAG, "cdi");
    ESP_RETURN_ON_ERROR(power_on(driver), TAG, "pon");
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_DRF, nullptr, 0, BUSY_REFRESH_TIMEOUT_MS), TAG, "drf");
    return send_command(driver, CMD_PTOUT, nullptr, 0);
}

static esp_err_t draw(void *context, const uint8_t *frame, ink_hal_display_rect_t window, ink_hal_display_mode_t mode,
                      ink_hal_display_timing_t *out_timing) {
    ink_drv_uc8253_t *driver = context;
    ESP_RETURN_ON_FALSE(frame && out_timing, ESP_ERR_INVALID_ARG, TAG, "null arg");
    uint16_t x0 = 0, x1 = INK_DRV_UC8253_WIDTH - 1, y0 = 0, y1 = INK_DRV_UC8253_HEIGHT - 1;
    if (mode == INK_HAL_DISPLAY_MODE_PARTIAL) {
        ESP_RETURN_ON_FALSE(window.width > 0
                                && window.height > 0
                                && window.x + window.width <= INK_DRV_UC8253_WIDTH
                                && window.y + window.height <= INK_DRV_UC8253_HEIGHT,
                            ESP_ERR_INVALID_ARG, TAG, "window");
        x0 = window.x & ~(X_ALIGN - 1);
        x1 = (window.x + window.width - 1) | (X_ALIGN - 1);
        y0 = window.y;
        y1 = window.y + window.height - 1;
        /* PTL cần VRED > VRST: nới thêm một hàng, ảnh hàng đó vẫn lấy từ frame */
        if (y0 == y1) {
            if (y1 < INK_DRV_UC8253_HEIGHT - 1) {
                y1++;
            } else {
                y0--;
            }
        }
    }

    int64_t mark_us = esp_timer_get_time();
    copy_window(driver, frame, x0, x1, y0, y1);
    ESP_RETURN_ON_ERROR(write_window(driver, CMD_DTM2, x0, x1, y0, y1), TAG, "new image");
    out_timing->spi_us = elapsed_us(&mark_us);

    if (mode == INK_HAL_DISPLAY_MODE_PARTIAL) {
        ESP_RETURN_ON_ERROR(refresh_partial(driver, x0, x1, y0, y1), TAG, "partial");
    } else {
        ESP_RETURN_ON_ERROR(refresh_full(driver), TAG, "full");
    }
    out_timing->panel_us = elapsed_us(&mark_us);

    if (mode == INK_HAL_DISPLAY_MODE_FULL || !driver->partial_options.keep_power_on) { ESP_RETURN_ON_ERROR(power_off(driver), TAG, "pof"); }
    /* controller không tự chép ảnh mới sang ảnh cũ */
    ESP_RETURN_ON_ERROR(write_window(driver, CMD_DTM1, x0, x1, y0, y1), TAG, "old image");
    out_timing->after_us = elapsed_us(&mark_us);
    return ESP_OK;
}

static const ink_hal_display_operations_t s_operations = {
    .draw = draw,
};

/* ---- Khởi tạo ---- */

static esp_err_t configure_pins(const ink_drv_uc8253_config_t *config) {
    const gpio_config_t output_config = {
        .pin_bit_mask = (1ULL << config->dc) | (1ULL << config->reset),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&output_config), TAG, "gpio_config out");
    ESP_RETURN_ON_ERROR(gpio_set_level(config->reset, 1), TAG, "reset high");
    const gpio_config_t busy_config = {
        .pin_bit_mask = 1ULL << config->busy,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_POSEDGE,
    };
    return gpio_config(&busy_config);
}

esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config, ink_hal_display_device_t *out_device) {
    ESP_RETURN_ON_FALSE(driver && config && out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    *driver = (ink_drv_uc8253_t){
        .config          = *config,
        .partial_options = {.pt_scan = true, .force_temperature = true, .temperature = 110},
    };
    ESP_RETURN_ON_ERROR(configure_pins(config), TAG, "pins");

    const spi_device_interface_config_t device_config = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode           = 0,
        .spics_io_num   = config->cs,
        .queue_size     = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(config->host, &device_config, &driver->spi), TAG, "spi_bus_add_device");

    driver->busy_ready = xSemaphoreCreateBinaryStatic(&driver->busy_ready_storage);
    /* Dịch vụ ngắt GPIO cài một lần cho cả chip; gọi lại trả
     * ESP_ERR_INVALID_STATE, coi như đã có. */
    const esp_err_t error = gpio_install_isr_service(0);
    ESP_RETURN_ON_FALSE(error == ESP_OK || error == ESP_ERR_INVALID_STATE, error, TAG, "gpio_install_isr_service");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(config->busy, on_busy_rise, driver), TAG, "gpio_isr_handler_add");

    ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset");

    *out_device = (ink_hal_display_device_t){
        .operations = &s_operations,
        .context    = driver,
        .capabilities =
            {
                           .width              = INK_DRV_UC8253_WIDTH,
                           .height             = INK_DRV_UC8253_HEIGHT,
                           .has_partial        = true,
                           .partial_x_align    = X_ALIGN,
                           .typical_full_ms    = TYPICAL_FULL_MS,
                           .typical_partial_ms = TYPICAL_PARTIAL_MS,
                           },
    };
    return ESP_OK;
}

void ink_drv_uc8253_set_partial_options(ink_drv_uc8253_t *driver, const ink_drv_uc8253_partial_options_t *options) {
    driver->partial_options = *options;
}
