/* Thiết kế: docs/design/display/display.md */

/* Lệnh và trình tự: docs/hardware/parts/weact-epaper-3.7/ (commands.md,
 * sequences.md, image-memory.md, waveform.md). */
#include "ink_drv_uc8253.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ink_drv_uc8253_profiles.h"
#include <string.h>

static const char *TAG = "ink_drv_uc8253";

#define DEFAULT_SPI_CLOCK_HZ (4 * 1000 * 1000)
#define READ_CLOCK_HZ        (1 * 1000 * 1000) /* chu kỳ SCL khi đọc ≥ 350 ns (UC8253 tr.61) */
#define ROW_BYTES            (INK_DRV_UC8253_WIDTH / 8)
#define X_ALIGN              8 /* PTL: HRST thấp 3 bit = 000, HRED = 111 */

#define CMD_PSR    0x00
#define CMD_PWR    0x01
#define CMD_POF    0x02
#define CMD_PON    0x04
#define CMD_DTM1   0x10
#define CMD_DRF    0x12
#define CMD_DTM2   0x13
#define CMD_LUTC   0x20 /* 0x20–0x24: LUTC, LUTWW, LUTKW, LUTWK, LUTKK */
#define CMD_LUTOPT 0x2A
#define CMD_PLL    0x30
#define CMD_TSC    0x40
#define CMD_CDI    0x50
#define CMD_VDCS   0x82
#define CMD_PTL    0x90
#define CMD_PTIN   0x91
#define CMD_PTOUT  0x92
#define CMD_ROTP   0xA2

/* PSR như GxEPD2: KW, quét lên, dịch phải; bit REG (D5) = 1 chọn LUT thanh ghi ở mọi lần vẽ. */
#define PSR_REGISTER   0x3F
#define PSR_SOFT_RESET 0x1E /* RST_N (D0) = 0: reset mềm, RAM ảnh thành rác (image-memory.md) */
#define PSR_SECOND     0x0D

#define PWR_SOURCE_INTERNAL 0x03 /* byte 1 của PWR: nguồn trong, giá trị mặc định (commands.md) */
#define PTL_SCAN_ALL        0x01 /* byte 7 của PTL: quét gate cả trong và ngoài window */
#define PTL_SCAN_WINDOW     0x00 /* chỉ quét trong window */
#define CDI_DRIVE_BORDER    0x97 /* viền làm mới theo bảng sóng */
#define CDI_FLOAT_BORDER    0xD7 /* viền nổi, như partial của GxEPD2 */

#define BUSY_SHORT_TIMEOUT_MS 200
#define BUSY_MARGIN_MS        100 /* timeout chờ DRF = drf_ms × 1,5 + chừng này */
#define LUT_SEND_BYTES        56  /* 0x20, 0x22–0x24 nhận 8 nhóm, KW dùng 6; 0x21 nhận 42 byte */
#define TABLE_WHITE_TO_WHITE  1

/* ---- Gói .inkw (tools/profile_gen.py) ---- */

#define BLOB_MAGIC         "INKW"
#define BLOB_VERSION       1
#define BLOB_HEADER_BYTES  8 /* magic 4, version 1, 0, độ dài payload 2 */
#define BLOB_PAYLOAD_BYTES (2 + INK_DRV_UC8253_NAME_BYTES + 3 + 4 + 2 + 2 + 2 + INK_DRV_UC8253_TABLE_COUNT * INK_DRV_UC8253_TABLE_BYTES)
#define BLOB_BYTES         (BLOB_HEADER_BYTES + BLOB_PAYLOAD_BYTES + 2)

/* ---- Profile ---- */

#define NO_PROFILE        (-1)
#define TEMPORARY_PROFILE INK_DRV_UC8253_PROFILE_COUNT
#define PROFILE_SLOTS     (INK_DRV_UC8253_PROFILE_COUNT + 1)

static_assert(PROFILE_SLOTS <= INK_DRV_UC8253_MAX_PROFILES, "tăng INK_DRV_UC8253_MAX_PROFILES");

/* nullptr nếu index ngoài danh sách hoặc profile tạm chưa nạp. */
static const ink_drv_uc8253_profile_t *profile_at(const ink_drv_uc8253_t *driver, size_t index) {
    if (index < INK_DRV_UC8253_PROFILE_COUNT) { return &ink_drv_uc8253_profiles[index]; }
    if (index == TEMPORARY_PROFILE) { return driver->temporary_profile; }
    return nullptr;
}

static ink_hal_display_profile_info_t profile_info(const ink_drv_uc8253_profile_t *profile) {
    return (ink_hal_display_profile_info_t){
        .id      = profile->id,
        .name    = profile->name,
        .source  = profile->source,
        .draw_ms = (uint32_t)profile->drf_ms * profile->scans,
    };
}

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

/* read: SPI 3 dây bán song công, đọc qua SDA (module không nối MISO) */
static esp_err_t add_device(ink_drv_uc8253_t *driver, bool read) {
    const uint32_t                      write_hz      = driver->config.spi_clock_hz ? driver->config.spi_clock_hz : DEFAULT_SPI_CLOCK_HZ;
    const spi_device_interface_config_t device_config = {
        .clock_speed_hz = read ? READ_CLOCK_HZ : (int)write_hz,
        .mode           = 0,
        .spics_io_num   = driver->config.cs,
        .queue_size     = 1,
        .flags          = read ? (SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX) : 0,
    };
    return spi_bus_add_device(driver->config.host, &device_config, &driver->spi);
}

/* ---- BUSY ---- */
/* BUSY thấp = bận. Lệnh có cờ BUSY: xoá semaphore trước khi gửi, rồi chờ edge lên. */

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
    /* BUSY có thể chưa kịp xuống ngay sau CS lên: chờ edge lên trước, rồi mới tin mức chân */
    ESP_RETURN_ON_FALSE(xSemaphoreTake(driver->busy_ready, pdMS_TO_TICKS(timeout_ms)) == pdTRUE, ESP_ERR_TIMEOUT, TAG,
                        "busy timeout cmd 0x%02x", command);
    return wait_ready(driver, timeout_ms);
}

/* ---- Đọc qua SDA ---- */

/*
 * Tạm đổi device SPI sang đọc, gửi `command`, chờ BUSY nếu `wait_busy`, rồi đọc
 * `length` byte vào window_buffer (DMA nhận theo word: đọc dư tới bội 4). Luôn
 * gắn lại device ghi.
 */
static esp_err_t read_after_command(ink_drv_uc8253_t *driver, uint8_t command, bool wait_busy, size_t length) {
    ESP_RETURN_ON_ERROR(spi_bus_remove_device(driver->spi), TAG, "remove write device");
    esp_err_t error = add_device(driver, true);
    if (error == ESP_OK) {
        error = wait_busy ? run_busy_command(driver, command, nullptr, 0, BUSY_SHORT_TIMEOUT_MS) : write_bytes(driver, false, &command, 1);
        if (error == ESP_OK) { error = gpio_set_level(driver->config.dc, 1); }
        if (error == ESP_OK) {
            spi_transaction_t transaction = {
                .rxlength  = ((length + 3) & ~(size_t)3) * 8,
                .rx_buffer = driver->window_buffer,
            };
            error = spi_device_transmit(driver->spi, &transaction);
        }
        const esp_err_t remove_error = spi_bus_remove_device(driver->spi);
        if (error == ESP_OK) { error = remove_error; }
    }
    const esp_err_t restore_error = add_device(driver, false);
    ESP_RETURN_ON_ERROR(error, TAG, "read 0x%02x", command);
    return restore_error;
}

/* ---- Khởi tạo controller ---- */

/* PSR (0x00): byte 1 chọn nguồn bảng sóng và reset mềm, byte 2 cố định. */
static esp_err_t send_panel_setting(ink_drv_uc8253_t *driver, uint8_t setting) {
    const uint8_t psr[2] = {setting, PSR_SECOND};
    return send_command(driver, CMD_PSR, psr, sizeof(psr));
}

/* Trạng thái sau reset mềm: RAM rác, nguồn tắt, chưa có bảng sóng. */
static void forget_controller_state(ink_drv_uc8253_t *driver) {
    driver->ram_swapped    = false;
    driver->ram_trusted    = false;
    driver->power_on       = false;
    driver->loaded_profile = NO_PROFILE;
    driver->needs_reset    = false;
}

static esp_err_t reset_controller(ink_drv_uc8253_t *driver) {
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.reset, 0), TAG, "reset low");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.reset, 1), TAG, "reset high");
    vTaskDelay(pdMS_TO_TICKS(10)); /* > 1 ms, lệnh gửi sớm hơn bị bỏ qua */
    ESP_RETURN_ON_ERROR(wait_ready(driver, BUSY_SHORT_TIMEOUT_MS), TAG, "after reset");

    const uint8_t soft_reset[2] = {PSR_SOFT_RESET, PSR_SECOND};
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PSR, soft_reset, sizeof(soft_reset), BUSY_SHORT_TIMEOUT_MS), TAG, "soft reset");
    forget_controller_state(driver);
    return send_panel_setting(driver, PSR_REGISTER);
}

/* ---- Nạp profile ---- */

/* PWR, VDCS, PLL, LUTOPT của profile; chờ BUSY sau LUTOPT. */
static esp_err_t send_voltages(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile) {
    const uint8_t pwr[] = {PWR_SOURCE_INTERNAL, profile->power[0], profile->power[1], profile->power[2], profile->power[3]};
    /* Byte 4–5 của LUTOPT lấy theo nhánh exp/uc8253-lut, đã chạy trên BB0. */
    const uint8_t lutopt[] = {profile->eopt, profile->xon[0], profile->xon[1], 0xFF, 0x00};
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PWR, pwr, sizeof(pwr)), TAG, "pwr");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_VDCS, &profile->vcom, 1), TAG, "vdcs");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PLL, &profile->pll, 1), TAG, "pll");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_LUTOPT, lutopt, sizeof(lutopt)), TAG, "lutopt");
    return wait_ready(driver, BUSY_SHORT_TIMEOUT_MS);
}

/* 5 bảng 0x20–0x24: LUTWW nhận 42 byte, bảng khác 56 byte (2 nhóm cuối bằng 0). */
static esp_err_t send_tables(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile) {
    for (size_t table = 0; table < INK_DRV_UC8253_TABLE_COUNT; table++) {
        static uint8_t bytes[LUT_SEND_BYTES]; /* DMA đọc: không để trên stack */
        memset(bytes, 0, sizeof(bytes));
        memcpy(bytes, profile->tables[table], INK_DRV_UC8253_TABLE_BYTES);
        const size_t length = table == TABLE_WHITE_TO_WHITE ? INK_DRV_UC8253_TABLE_BYTES : LUT_SEND_BYTES;
        ESP_RETURN_ON_ERROR(send_command(driver, (uint8_t)(CMD_LUTC + table), bytes, length), TAG, "lut %u", (unsigned)table);
    }
    return ESP_OK;
}

static esp_err_t load_profile(ink_drv_uc8253_t *driver, size_t index, const ink_drv_uc8253_profile_t *profile) {
    if (driver->loaded_profile == (int)index) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(send_voltages(driver, profile), TAG, "voltages");
    ESP_RETURN_ON_ERROR(send_tables(driver, profile), TAG, "tables");
    driver->loaded_profile = (int)index;
    return ESP_OK;
}

/* ---- Vẽ ---- */

typedef struct {
    uint16_t x0, x1, y0, y1; /* gồm cả mép; x0, x1 + 1 là bội của X_ALIGN */
} window_t;

static window_t whole_screen(void) {
    return (window_t){.x0 = 0, .x1 = INK_DRV_UC8253_WIDTH - 1, .y0 = 0, .y1 = INK_DRV_UC8253_HEIGHT - 1};
}

/* Vào chế độ partial với window đã căn mép (PTIN rồi PTL). Bên gọi gửi PTOUT khi xong. */
static esp_err_t enter_window(ink_drv_uc8253_t *driver, const window_t *window, bool pt_scan) {
    /* PTL 7 byte (image-memory.md): HRST, HRED, VRST cao/thấp, VRED cao/thấp, PT_SCAN */
    const uint8_t ptl[7] = {
        (uint8_t)window->x0,
        (uint8_t)window->x1,
        (uint8_t)(window->y0 >> 8),
        (uint8_t)(window->y0 & 0xFF),
        (uint8_t)(window->y1 >> 8),
        (uint8_t)(window->y1 & 0xFF),
        pt_scan ? PTL_SCAN_ALL : PTL_SCAN_WINDOW,
    };
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PTIN, nullptr, 0), TAG, "ptin");
    return send_command(driver, CMD_PTL, ptl, sizeof(ptl));
}

/* Cắt window của một mặt phẳng vào window_buffer rồi ghi vào một vùng RAM ảnh. */
static esp_err_t write_plane(ink_drv_uc8253_t *driver, uint8_t ram_command, const uint8_t *plane, const window_t *window) {
    const size_t row_bytes = (window->x1 - window->x0 + 1) / 8;
    for (uint16_t y = window->y0; y <= window->y1; y++) {
        memcpy(&driver->window_buffer[(y - window->y0) * row_bytes], &plane[y * ROW_BYTES + window->x0 / 8], row_bytes);
    }
    ESP_RETURN_ON_ERROR(enter_window(driver, window, true), TAG, "window");
    ESP_RETURN_ON_ERROR(send_command(driver, ram_command, driver->window_buffer, row_bytes * (window->y1 - window->y0 + 1)), TAG, "ram");
    return send_command(driver, CMD_PTOUT, nullptr, 0);
}

/* Datasheet không nói PON khi đang bật có kéo BUSY xuống không; chỉ gửi khi đang tắt. */
static esp_err_t power_on(ink_drv_uc8253_t *driver) {
    if (driver->power_on) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PON, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pon");
    driver->power_on = true;
    return ESP_OK;
}

static esp_err_t send_power_off(ink_drv_uc8253_t *driver) {
    if (!driver->power_on) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_POF, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pof");
    driver->power_on = false;
    return ESP_OK;
}

/* Một lần DRF. Ngoài chế độ partial: cả màn, không PTIN. */
static esp_err_t refresh(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile, const window_t *window,
                         const ink_hal_display_draw_config_t *config) {
    if (config->partial_mode) { ESP_RETURN_ON_ERROR(enter_window(driver, window, config->pt_scan), TAG, "window"); }
    const uint8_t cdi = config->drive_border ? CDI_DRIVE_BORDER : CDI_FLOAT_BORDER;
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CDI, &cdi, 1), TAG, "cdi");
    ESP_RETURN_ON_ERROR(power_on(driver), TAG, "pon");
    const uint32_t timeout_ms = (uint32_t)profile->drf_ms * 3 / 2 + BUSY_MARGIN_MS;
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_DRF, nullptr, 0, timeout_ms), TAG, "drf");
    /* Sau mỗi DRF controller đổi vai hai vùng RAM (image-memory.md) */
    driver->ram_swapped = !driver->ram_swapped;
    return config->partial_mode ? send_command(driver, CMD_PTOUT, nullptr, 0) : ESP_OK;
}

static uint32_t elapsed_us(int64_t *since_us) {
    const int64_t now_us = esp_timer_get_time();
    const int64_t result = now_us - *since_us;
    *since_us            = now_us;
    return (uint32_t)result;
}

/* Profile tạm do task khác nạp: lấy ra trước lần vẽ; quên profile đang nạp nếu nó là profile tạm. */
static void apply_temporary(ink_drv_uc8253_t *driver) {
    taskENTER_CRITICAL(&driver->temporary_lock);
    const bool pending = driver->temporary_pending;
    if (pending) {
        driver->temporary = driver->temporary_request;
        memcpy(driver->temporary_name, driver->temporary_request_name, sizeof(driver->temporary_name));
        driver->temporary_pending = false;
    }
    taskEXIT_CRITICAL(&driver->temporary_lock);
    if (!pending) { return; }
    driver->temporary.name                  = driver->temporary_name;
    driver->temporary_profile               = &driver->temporary;
    driver->profile_info[TEMPORARY_PROFILE] = profile_info(&driver->temporary);
    if (driver->loaded_profile == TEMPORARY_PROFILE) { driver->loaded_profile = NO_PROFILE; }
}

/* Window đã căn mép, hoặc cả màn khi ngoài chế độ partial hay khi RAM không còn đáng tin. */
static esp_err_t make_window(const ink_drv_uc8253_t *driver, ink_hal_display_rect_t rect, bool partial_mode, window_t *out_window) {
    if (!partial_mode || !driver->ram_trusted) {
        *out_window = whole_screen();
        return ESP_OK;
    }
    const bool not_empty = rect.width > 0 && rect.height > 0;
    const bool inside    = rect.x + rect.width <= INK_DRV_UC8253_WIDTH && rect.y + rect.height <= INK_DRV_UC8253_HEIGHT;
    ESP_RETURN_ON_FALSE(not_empty && inside, ESP_ERR_INVALID_ARG, TAG, "window");
    window_t window = {
        .x0 = rect.x & ~(X_ALIGN - 1),
        .x1 = (rect.x + rect.width - 1) | (X_ALIGN - 1),
        .y0 = rect.y,
        .y1 = rect.y + rect.height - 1,
    };
    /* PTL cần VRED > VRST: nới thêm một hàng, ảnh hàng đó vẫn lấy từ mặt phẳng */
    if (window.y0 == window.y1) {
        if (window.y1 < INK_DRV_UC8253_HEIGHT - 1) {
            window.y1++;
        } else {
            window.y0--;
        }
    }
    *out_window = window;
    return ESP_OK;
}

static esp_err_t draw_planes(ink_drv_uc8253_t *driver, const uint8_t *plane_new, const uint8_t *plane_old, ink_hal_display_rect_t rect,
                             size_t index, const ink_hal_display_draw_config_t *config, ink_hal_display_timing_t *out_timing) {
    if (driver->needs_reset) { ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset"); }
    apply_temporary(driver);
    const ink_drv_uc8253_profile_t *profile = profile_at(driver, index);
    ESP_RETURN_ON_FALSE(profile != nullptr, index == TEMPORARY_PROFILE ? ESP_ERR_INVALID_STATE : ESP_ERR_INVALID_ARG, TAG, "profile %u",
                        (unsigned)index);
    window_t window;
    ESP_RETURN_ON_ERROR(make_window(driver, rect, config->partial_mode, &window), TAG, "window");

    int64_t mark_us = esp_timer_get_time();
    *out_timing     = (ink_hal_display_timing_t){};
    ESP_RETURN_ON_ERROR(load_profile(driver, index, profile), TAG, "profile %s", profile->name);
    out_timing->spi_us += elapsed_us(&mark_us);

    driver->needs_reset = true; /* lỗi giữa chừng thì không còn biết lượt */
    for (uint32_t scan = 0; scan < profile->scans; scan++) {
        /* Số DRF từ reset chẵn: mới vào 0x13, cũ vào 0x10; lẻ thì ngược lại. Ghi cả
         * hai trước DRF như luồng của hãng (GDEY037T03 tr.39). */
        const uint8_t new_command = driver->ram_swapped ? CMD_DTM1 : CMD_DTM2;
        const uint8_t old_command = driver->ram_swapped ? CMD_DTM2 : CMD_DTM1;
        ESP_RETURN_ON_ERROR(write_plane(driver, new_command, plane_new, &window), TAG, "new plane");
        ESP_RETURN_ON_ERROR(write_plane(driver, old_command, plane_old, &window), TAG, "old plane");
        out_timing->spi_us += elapsed_us(&mark_us);
        ESP_RETURN_ON_ERROR(refresh(driver, profile, &window, config), TAG, "refresh");
        out_timing->panel_us += elapsed_us(&mark_us);
    }
    driver->needs_reset  = false;
    driver->ram_trusted  = true;
    out_timing->after_us = elapsed_us(&mark_us);
    return ESP_OK;
}

/* ---- Bảng hàm ink_hal_display ---- */

static esp_err_t draw(void *context, const uint8_t *plane_new, const uint8_t *plane_old, ink_hal_display_rect_t window, size_t profile,
                      const ink_hal_display_draw_config_t *config, ink_hal_display_timing_t *out_timing) {
    ESP_RETURN_ON_FALSE(context && plane_new && plane_old && config && out_timing, ESP_ERR_INVALID_ARG, TAG, "arg");
    return draw_planes(context, plane_new, plane_old, window, profile, config, out_timing);
}

static esp_err_t power_off(void *context) {
    ESP_RETURN_ON_FALSE(context, ESP_ERR_INVALID_ARG, TAG, "arg");
    return send_power_off(context);
}

/* Lệnh 0x40 kéo BUSY xuống trong lúc đo; byte đầu là TS[7:0], °C có dấu (UC8253 tr.27).
 * Chưa chạy trên BB0 khi REG = 1. */
static esp_err_t read_temperature(void *context, int8_t *out_celsius) {
    ink_drv_uc8253_t *driver = context;
    ESP_RETURN_ON_FALSE(driver && out_celsius, ESP_ERR_INVALID_ARG, TAG, "arg");
    ESP_RETURN_ON_ERROR(read_after_command(driver, CMD_TSC, true, 2), TAG, "tsc");
    *out_celsius = (int8_t)driver->window_buffer[0];
    return ESP_OK;
}

static const ink_hal_display_operations_t s_operations = {
    .draw             = draw,
    .power_off        = power_off,
    .read_temperature = read_temperature,
};

/* ---- Firmware test: profile tạm, đọc OTP ---- */

static uint16_t read_u16(const uint8_t *bytes) {
    return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

/* CRC-16/CCITT-FALSE, như tools/profile_gen.py. */
static uint16_t crc16(const uint8_t *bytes, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)(bytes[i] << 8);
        for (int bit = 0; bit < 8; bit++) { crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1); }
    }
    return crc;
}

/* Bố cục payload: id, tên, nguồn, số lần quét, PLL, PWR (4), VCOM, EOPT, XON (2), DRF ms, 5 bảng. */
static void parse_blob(const uint8_t *payload, ink_drv_uc8253_profile_t *out_profile, char *out_name) {
    const uint8_t *cursor = payload;
    out_profile->id       = read_u16(cursor);
    cursor += 2;
    memcpy(out_name, cursor, INK_DRV_UC8253_NAME_BYTES);
    out_name[INK_DRV_UC8253_NAME_BYTES - 1] = '\0';
    cursor += INK_DRV_UC8253_NAME_BYTES;
    out_profile->source = *cursor++;
    out_profile->scans  = *cursor++;
    out_profile->pll    = *cursor++;
    memcpy(out_profile->power, cursor, sizeof(out_profile->power));
    cursor += sizeof(out_profile->power);
    out_profile->vcom = *cursor++;
    out_profile->eopt = *cursor++;
    memcpy(out_profile->xon, cursor, sizeof(out_profile->xon));
    cursor += sizeof(out_profile->xon);
    out_profile->drf_ms = read_u16(cursor);
    cursor += 2;
    memcpy(out_profile->tables, cursor, sizeof(out_profile->tables));
}

esp_err_t ink_drv_uc8253_load_temporary(ink_drv_uc8253_t *driver, const uint8_t *blob, size_t length) {
    ESP_RETURN_ON_FALSE(driver && blob && length == BLOB_BYTES, ESP_ERR_INVALID_ARG, TAG, "blob length %u", (unsigned)length);
    ESP_RETURN_ON_FALSE(memcmp(blob, BLOB_MAGIC, 4) == 0 && blob[4] == BLOB_VERSION, ESP_ERR_INVALID_ARG, TAG, "blob magic");
    ESP_RETURN_ON_FALSE(read_u16(&blob[6]) == BLOB_PAYLOAD_BYTES, ESP_ERR_INVALID_ARG, TAG, "blob payload");
    const uint16_t crc = read_u16(&blob[BLOB_BYTES - 2]);
    ESP_RETURN_ON_FALSE(crc16(&blob[4], BLOB_BYTES - 6) == crc, ESP_ERR_INVALID_ARG, TAG, "blob crc");

    ink_drv_uc8253_profile_t profile;
    char                     name[INK_DRV_UC8253_NAME_BYTES];
    parse_blob(&blob[BLOB_HEADER_BYTES], &profile, name);
    ESP_RETURN_ON_FALSE(profile.scans >= 1 && profile.scans <= 4 && profile.drf_ms > 0, ESP_ERR_INVALID_ARG, TAG, "blob scans");
    taskENTER_CRITICAL(&driver->temporary_lock);
    driver->temporary_request = profile;
    memcpy(driver->temporary_request_name, name, sizeof(name));
    driver->temporary_pending = true;
    taskEXIT_CRITICAL(&driver->temporary_lock);
    return ESP_OK;
}

esp_err_t ink_drv_uc8253_read_otp(ink_drv_uc8253_t *driver, uint8_t *out, size_t length) {
    ESP_RETURN_ON_FALSE(driver && out && length >= INK_DRV_UC8253_OTP_BYTES, ESP_ERR_INVALID_ARG, TAG, "arg");
    ESP_RETURN_ON_ERROR(read_after_command(driver, CMD_ROTP, false, INK_DRV_UC8253_OTP_BYTES + 1), TAG, "rotp");
    memcpy(out, &driver->window_buffer[1], INK_DRV_UC8253_OTP_BYTES); /* byte đầu của ROTP là byte giả */
    return ESP_OK;
}

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

/* Danh sách profile cố định, rồi profile tạm (id 0, "tmp") tới khi bench nạp. */
static void fill_profile_info(ink_drv_uc8253_t *driver) {
    for (size_t i = 0; i < INK_DRV_UC8253_PROFILE_COUNT; i++) { driver->profile_info[i] = profile_info(&ink_drv_uc8253_profiles[i]); }
    driver->profile_info[TEMPORARY_PROFILE] =
        (ink_hal_display_profile_info_t){.id = 0, .name = "tmp", .source = INK_HAL_DISPLAY_SOURCE_HAND};
}

esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config, ink_hal_display_device_t *out_device) {
    ESP_RETURN_ON_FALSE(driver && config && out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    *driver = (ink_drv_uc8253_t){
        .config         = *config,
        .loaded_profile = NO_PROFILE,
        .temporary_lock = portMUX_INITIALIZER_UNLOCKED,
    };
    fill_profile_info(driver);

    ESP_RETURN_ON_ERROR(configure_pins(config), TAG, "pins");
    ESP_RETURN_ON_ERROR(add_device(driver, false), TAG, "spi_bus_add_device");
    driver->busy_ready = xSemaphoreCreateBinaryStatic(&driver->busy_ready_storage);
    /* Dịch vụ ngắt GPIO do bsp cài (ink_bsp_init). */
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(config->busy, on_busy_rise, driver), TAG, "gpio_isr_handler_add");
    ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset");

    const ink_hal_display_capabilities_t capabilities = {
        .width            = INK_DRV_UC8253_WIDTH,
        .height           = INK_DRV_UC8253_HEIGHT,
        .x_align          = X_ALIGN,
        .profiles         = driver->profile_info,
        .profile_count    = PROFILE_SLOTS,
        .purpose_defaults = ink_drv_uc8253_purpose_defaults,
    };

    *out_device = (ink_hal_display_device_t){
        .operations   = &s_operations,
        .context      = driver,
        .capabilities = capabilities,
    };
    return ESP_OK;
}
