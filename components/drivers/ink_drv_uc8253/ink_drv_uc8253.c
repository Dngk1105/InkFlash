/* Design: docs/design/display/display.md */

/* Commands and sequences: docs/hardware/parts/weact-epaper-3.7/ (commands.md,
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
#define READ_CLOCK_HZ        (1 * 1000 * 1000) /* SCL period when reading >= 350 ns (UC8253 p.61) */
#define ROW_BYTES            (INK_DRV_UC8253_WIDTH / 8)
#define X_ALIGN              8 /* PTL: low 3 bits of HRST = 000, HRED = 111 */

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

/* PSR as in GxEPD2: KW, scan up, shift right; REG bit (D5) = 1 selects register LUTs for every draw. */
#define PSR_REGISTER   0x3F
#define PSR_SOFT_RESET 0x1E /* RST_N (D0) = 0: soft reset; image RAM becomes garbage (image-memory.md) */
#define PSR_SECOND     0x0A /* TIEG 0, NORG 1, VC_LUTZ 0: default 0x0D (TIEG 1) darkens white areas over time after POF (refresh.md) */

#define PWR_SOURCE_INTERNAL 0x03 /* PWR byte 1: internal power, default value (commands.md) */
#define PTL_SCAN_ALL        0x01 /* PTL byte 7: scan gates inside and outside the window */
#define PTL_SCAN_WINDOW     0x00 /* scan inside the window only */
#define CDI_DRIVE_BORDER    0x97 /* border driven by the waveform */
#define CDI_FLOAT_BORDER    0xD7 /* border floats, as in GxEPD2 partial */

#define BUSY_SHORT_TIMEOUT_MS 200
#define BUSY_MARGIN_MS        100 /* DRF wait timeout = drf_ms × 1.5 + this */
#define LUT_SEND_BYTES        56  /* 0x20, 0x22-0x24 take 8 groups, KW uses 6; 0x21 takes 42 bytes */
#define TABLE_WHITE_TO_WHITE  1

/* ---- Profile ---- */

#define NO_PROFILE    (-1)
#define PROFILE_SLOTS INK_DRV_UC8253_PROFILE_COUNT

static_assert(PROFILE_SLOTS <= INK_DRV_UC8253_MAX_PROFILES, "tăng INK_DRV_UC8253_MAX_PROFILES");

/* NULL if index is out of range. */
static const ink_drv_uc8253_profile_t *profile_at(size_t index) {
    return index < INK_DRV_UC8253_PROFILE_COUNT ? &ink_drv_uc8253_profiles[index] : nullptr;
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
    /* large image: wait on the interrupt, not the CPU */
    transaction.tx_buffer = bytes;
    return spi_device_transmit(driver->spi, &transaction);
}

static esp_err_t send_command(ink_drv_uc8253_t *driver, uint8_t command, const uint8_t *data, size_t length) {
    ESP_RETURN_ON_ERROR(write_bytes(driver, false, &command, 1), TAG, "cmd 0x%02x", command);
    if (length > 0) { ESP_RETURN_ON_ERROR(write_bytes(driver, true, data, length), TAG, "data 0x%02x", command); }
    return ESP_OK;
}

/* read: 3-wire half-duplex SPI, read through SDA (the module has no MISO) */
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
/* BUSY low = busy. Commands that pull BUSY: clear the semaphore before sending, then wait for the rising edge. */

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

/* Send a command that pulls BUSY low, then sleep until BUSY goes high. */
static esp_err_t run_busy_command(ink_drv_uc8253_t *driver, uint8_t command, const uint8_t *data, size_t length, uint32_t timeout_ms) {
    xSemaphoreTake(driver->busy_ready, 0);
    ESP_RETURN_ON_ERROR(send_command(driver, command, data, length), TAG, "busy cmd");
    /* BUSY may not be low yet right after CS goes high: wait for the rising edge first, then trust the pin level */
    ESP_RETURN_ON_FALSE(xSemaphoreTake(driver->busy_ready, pdMS_TO_TICKS(timeout_ms)) == pdTRUE, ESP_ERR_TIMEOUT, TAG,
                        "busy timeout cmd 0x%02x", command);
    return wait_ready(driver, timeout_ms);
}

/* ---- Read through SDA ---- */

/*
 * Switch the SPI device to read, send `command`, wait for BUSY if `wait_busy`, then read
 * `length` bytes into window_buffer (DMA receives whole words: reads up to a multiple of 4).
 * Always re-attaches the write device.
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

/* ---- Controller init ---- */

/* PSR (0x00): byte 1 selects the waveform source and soft reset; byte 2 is fixed. */
static esp_err_t send_panel_setting(ink_drv_uc8253_t *driver, uint8_t setting) {
    const uint8_t psr[2] = {setting, PSR_SECOND};
    return send_command(driver, CMD_PSR, psr, sizeof(psr));
}

/* State after soft reset: image RAM garbage, analog power off, no waveform loaded. */
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
    vTaskDelay(pdMS_TO_TICKS(10)); /* > 1 ms; earlier commands are ignored */
    ESP_RETURN_ON_ERROR(wait_ready(driver, BUSY_SHORT_TIMEOUT_MS), TAG, "after reset");

    const uint8_t soft_reset[2] = {PSR_SOFT_RESET, PSR_SECOND};
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PSR, soft_reset, sizeof(soft_reset), BUSY_SHORT_TIMEOUT_MS), TAG, "soft reset");
    forget_controller_state(driver);
    return send_panel_setting(driver, PSR_REGISTER);
}

/* ---- Load profile ---- */

/* PWR, VDCS, PLL, LUTOPT of the profile; wait for BUSY after LUTOPT. */
static esp_err_t send_voltages(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile) {
    const uint8_t pwr[] = {PWR_SOURCE_INTERNAL, profile->power[0], profile->power[1], profile->power[2], profile->power[3]};
    /* LUTOPT bytes 4-5 from branch exp/uc8253-lut, verified on BB0. */
    const uint8_t lutopt[] = {profile->eopt, profile->xon[0], profile->xon[1], 0xFF, 0x00};
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PWR, pwr, sizeof(pwr)), TAG, "pwr");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_VDCS, &profile->vcom, 1), TAG, "vdcs");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PLL, &profile->pll, 1), TAG, "pll");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_LUTOPT, lutopt, sizeof(lutopt)), TAG, "lutopt");
    return wait_ready(driver, BUSY_SHORT_TIMEOUT_MS);
}

/* 5 LUTs 0x20-0x24: LUTWW takes 42 bytes, the others 56 bytes (last 2 groups zero). */
static esp_err_t send_tables(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile) {
    for (size_t table = 0; table < INK_DRV_UC8253_TABLE_COUNT; table++) {
        static uint8_t bytes[LUT_SEND_BYTES]; /* read by DMA: not on the stack */
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

/* ---- Draw ---- */

typedef struct {
    uint16_t x0, x1, y0, y1; /* edges inclusive; x0 and x1 + 1 are multiples of X_ALIGN */
} window_t;

static window_t whole_screen(void) {
    return (window_t){.x0 = 0, .x1 = INK_DRV_UC8253_WIDTH - 1, .y0 = 0, .y1 = INK_DRV_UC8253_HEIGHT - 1};
}

/* Enter partial mode with the aligned window (PTIN, then PTL). The caller sends PTOUT when done. */
static esp_err_t enter_window(ink_drv_uc8253_t *driver, const window_t *window, bool scan_outside_window) {
    /* PTL, 7 bytes (image-memory.md): HRST, HRED, VRST high/low, VRED high/low, PT_SCAN */
    const uint8_t ptl[7] = {
        (uint8_t)window->x0,
        (uint8_t)window->x1,
        (uint8_t)(window->y0 >> 8),
        (uint8_t)(window->y0 & 0xFF),
        (uint8_t)(window->y1 >> 8),
        (uint8_t)(window->y1 & 0xFF),
        scan_outside_window ? PTL_SCAN_ALL : PTL_SCAN_WINDOW,
    };
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PTIN, nullptr, 0), TAG, "ptin");
    return send_command(driver, CMD_PTL, ptl, sizeof(ptl));
}

/* Copy the window of one plane into window_buffer, then write it to one image RAM area. */
static esp_err_t write_plane(ink_drv_uc8253_t *driver, uint8_t ram_command, const uint8_t *plane, const window_t *window) {
    const size_t row_bytes = (window->x1 - window->x0 + 1) / 8;
    for (uint16_t y = window->y0; y <= window->y1; y++) {
        memcpy(&driver->window_buffer[(y - window->y0) * row_bytes], &plane[y * ROW_BYTES + window->x0 / 8], row_bytes);
    }
    ESP_RETURN_ON_ERROR(enter_window(driver, window, true), TAG, "window");
    ESP_RETURN_ON_ERROR(send_command(driver, ram_command, driver->window_buffer, row_bytes * (window->y1 - window->y0 + 1)), TAG, "ram");
    return send_command(driver, CMD_PTOUT, nullptr, 0);
}

/* The datasheet does not say whether PON pulls BUSY low when power is already on; send it only when off. Adds to `power_us`. */
static esp_err_t power_on(ink_drv_uc8253_t *driver, uint32_t *power_us) {
    if (driver->power_on) { return ESP_OK; }
    const int64_t start_us = esp_timer_get_time();
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PON, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pon");
    *power_us += (uint32_t)(esp_timer_get_time() - start_us);
    driver->power_on = true;
    return ESP_OK;
}

static esp_err_t send_power_off(ink_drv_uc8253_t *driver) {
    if (!driver->power_on) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_POF, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pof");
    driver->power_on = false;
    return ESP_OK;
}

/* One DRF. Outside partial mode: whole panel, no PTIN. */
static esp_err_t refresh(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile, const window_t *window,
                         const ink_hal_display_draw_config_t *config, uint32_t *power_us) {
    if (config->partial_mode) { ESP_RETURN_ON_ERROR(enter_window(driver, window, config->scan_outside_window), TAG, "window"); }
    const uint8_t cdi = config->drive_border ? CDI_DRIVE_BORDER : CDI_FLOAT_BORDER;
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CDI, &cdi, 1), TAG, "cdi");
    ESP_RETURN_ON_ERROR(power_on(driver, power_us), TAG, "pon");
    const uint32_t timeout_ms = (uint32_t)profile->drf_ms * 3 / 2 + BUSY_MARGIN_MS;
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_DRF, nullptr, 0, timeout_ms), TAG, "drf");
    /* After each DRF the controller swaps the roles of the two RAM areas (image-memory.md) */
    driver->ram_swapped = !driver->ram_swapped;
    return config->partial_mode ? send_command(driver, CMD_PTOUT, nullptr, 0) : ESP_OK;
}

static uint32_t elapsed_us(int64_t *since_us) {
    const int64_t now_us = esp_timer_get_time();
    const int64_t result = now_us - *since_us;
    *since_us            = now_us;
    return (uint32_t)result;
}

/* Aligned window, or the whole panel outside partial mode or when image RAM is not trusted. */
static esp_err_t make_window(const ink_drv_uc8253_t *driver, ink_display_rect_t rect, bool partial_mode, window_t *out_window) {
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
    /* PTL needs VRED > VRST: grow by one row; that row's image still comes from the planes */
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

static esp_err_t draw_planes(ink_drv_uc8253_t *driver, const uint8_t *plane_new, const uint8_t *plane_old, ink_display_rect_t rect,
                             size_t index, const ink_hal_display_draw_config_t *config, ink_hal_display_timing_t *out_timing) {
    if (driver->needs_reset) { ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset"); }
    const ink_drv_uc8253_profile_t *profile = profile_at(index);
    ESP_RETURN_ON_FALSE(profile != nullptr, ESP_ERR_INVALID_ARG, TAG, "profile %u", (unsigned)index);
    window_t window;
    ESP_RETURN_ON_ERROR(make_window(driver, rect, config->partial_mode, &window), TAG, "window");

    int64_t mark_us = esp_timer_get_time();
    *out_timing     = (ink_hal_display_timing_t){};
    ESP_RETURN_ON_ERROR(load_profile(driver, index, profile), TAG, "profile %s", profile->name);
    out_timing->spi_us += elapsed_us(&mark_us);

    driver->needs_reset = true; /* failure midway: RAM swap state unknown */
    for (uint32_t scan = 0; scan < profile->scans; scan++) {
        /* Even DRF count since reset: new -> 0x13, old -> 0x10; odd: the reverse. Write both
         * before the DRF, as in the vendor flow (GDEY037T03 p.39). */
        const uint8_t new_command = driver->ram_swapped ? CMD_DTM1 : CMD_DTM2;
        const uint8_t old_command = driver->ram_swapped ? CMD_DTM2 : CMD_DTM1;
        ESP_RETURN_ON_ERROR(write_plane(driver, new_command, plane_new, &window), TAG, "new plane");
        ESP_RETURN_ON_ERROR(write_plane(driver, old_command, plane_old, &window), TAG, "old plane");
        out_timing->spi_us += elapsed_us(&mark_us);
        const uint32_t power_before = out_timing->power_us;
        ESP_RETURN_ON_ERROR(refresh(driver, profile, &window, config, &out_timing->power_us), TAG, "refresh");
        out_timing->panel_us += elapsed_us(&mark_us) - (out_timing->power_us - power_before);
    }
    driver->needs_reset      = false;
    driver->ram_trusted      = true;
    out_timing->post_scan_us = elapsed_us(&mark_us);
    return ESP_OK;
}

/* ---- ink_hal_display operations ---- */

static esp_err_t draw(void *context, const uint8_t *plane_new, const uint8_t *plane_old, ink_display_rect_t window, size_t profile_index,
                      const ink_hal_display_draw_config_t *config, ink_hal_display_timing_t *out_timing) {
    ESP_RETURN_ON_FALSE(context && plane_new && plane_old && config && out_timing, ESP_ERR_INVALID_ARG, TAG, "arg");
    return draw_planes(context, plane_new, plane_old, window, profile_index, config, out_timing);
}

static esp_err_t power_off(void *context) {
    ESP_RETURN_ON_FALSE(context, ESP_ERR_INVALID_ARG, TAG, "arg");
    return send_power_off(context);
}

/* Command 0x40 pulls BUSY low while measuring; first byte is TS[7:0], signed °C (UC8253 p.27).
 * Verified on BB0 with REG = 1 (2026-11-04). */
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

/* ---- Init ---- */

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

static void fill_profile_info(ink_drv_uc8253_t *driver) {
    for (size_t i = 0; i < INK_DRV_UC8253_PROFILE_COUNT; i++) { driver->profile_info[i] = profile_info(&ink_drv_uc8253_profiles[i]); }
}

esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config, ink_hal_display_device_t *out_device) {
    ESP_RETURN_ON_FALSE(driver && config && out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    *driver = (ink_drv_uc8253_t){
        .config         = *config,
        .loaded_profile = NO_PROFILE,
    };
    fill_profile_info(driver);

    ESP_RETURN_ON_ERROR(configure_pins(config), TAG, "pins");
    ESP_RETURN_ON_ERROR(add_device(driver, false), TAG, "spi_bus_add_device");
    driver->busy_ready = xSemaphoreCreateBinaryStatic(&driver->busy_ready_storage);
    /* The GPIO ISR service is installed by the BSP (ink_bsp_init). */
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(config->busy, on_busy_rise, driver), TAG, "gpio_isr_handler_add");
    ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset");

    const ink_hal_display_capabilities_t capabilities = {
        .width               = INK_DRV_UC8253_WIDTH,
        .height              = INK_DRV_UC8253_HEIGHT,
        .pixel_format        = INK_DISPLAY_PIXEL_FORMAT_K1,
        .x_align             = X_ALIGN,
        .profiles            = driver->profile_info,
        .profile_count       = PROFILE_SLOTS,
        .draw_defaults       = ink_drv_uc8253_draw_defaults,
        .cleanup_defaults    = ink_drv_uc8253_cleanup_defaults,
        .strong_push_default = &ink_drv_uc8253_strong_push_default,
    };

    *out_device = (ink_hal_display_device_t){
        .operations   = &s_operations,
        .context      = driver,
        .capabilities = capabilities,
    };
    return ESP_OK;
}
