/* Thiết kế: docs/design/display/display.md */

/* Lệnh và trình tự: docs/hardware/parts/weact-epaper-3.7/ (commands.md,
 * sequences.md, image-memory.md, waveform.md). */
#include "ink_drv_uc8253.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ink_waveform.h"
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
#define CMD_CCSET  0xE0
#define CMD_TSSET  0xE5

/* PSR như GxEPD2: KW, quét lên, dịch phải; bit REG (D5) chọn LUT thanh ghi. */
#define PSR_OTP      0x1F
#define PSR_REGISTER 0x3F
#define PSR_SECOND   0x0D

#define BUSY_SHORT_TIMEOUT_MS   200
#define BUSY_REFRESH_TIMEOUT_MS 5000

/* ---- Bố cục OTP, chế độ KW (UC8253 tr.47–52, waveform.md) ---- */

#define OTP_LOCK       0xA5
#define OTP_TR_FIRST   0x2A
#define OTP_TR8        8
#define OTP_READ_BYTES (1 + OTP_TR_FIRST + (OTP_TR8 + 1) * INK_DRV_UC8253_TR_BYTES) /* byte đầu của ROTP là byte giả */
#define TR_FRAME_RATE  0x00
#define TR_VG          0x01
#define TR_VSH         0x02
#define TR_VSL         0x03
#define TR_VDHR        0x04
#define TR_VCOM        0x05
#define TR_EOPT        0x06
#define TR_XON_LOW     0x07
#define TR_XON_HIGH    0x08
#define TR_LUT_FIRST   0x09
#define TR8_FRAMES     15
#define LUT_SEND_BYTES 56 /* 0x20, 0x22–0x24 nhận 8 nhóm, KW dùng 6; 0x21 nhận 42 byte */

/* ---- Profile ---- */

#define TUNE_PROFILE    (INK_DRV_UC8253_PROFILE_COUNT - 1)
#define NO_PROFILE      (-1)
#define CDI_FULL        0x97
#define CDI_PARTIAL     0xD7
#define DEFAULT_FULL    1 /* otp90_full */
#define DEFAULT_PARTIAL 2 /* otp110 */

/* Các bảng sóng đã đo trên BB0 (refresh.md). Mặc định chọn sau, khi so bằng mắt xong. */
static const ink_drv_uc8253_profile_t s_presets[TUNE_PROFILE] = {
    {.name = "otp_std_full", .source = INK_DRV_UC8253_SOURCE_OTP_SENSOR, .otp_drf_ms = 2600, .cdi = CDI_FULL},
    {.name = "otp90_full", .source = INK_DRV_UC8253_SOURCE_OTP_FORCED, .temperature = 90, .otp_drf_ms = 2055, .cdi = CDI_FULL},
    {.name        = "otp110",
     .source      = INK_DRV_UC8253_SOURCE_OTP_FORCED,
     .temperature = 110,
     .otp_drf_ms  = 344,
     .pt_scan     = true,
     .cdi         = CDI_PARTIAL},
    {.name = "lut15f50", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 0, .frame_rate_hz = 50, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name = "lut15f65", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 0, .frame_rate_hz = 65, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name = "lut12f50", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 12, .frame_rate_hz = 50, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name = "lut10f50", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 10, .frame_rate_hz = 50, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name = "lut15f100", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 0, .frame_rate_hz = 100, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name = "lut6f50", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 6, .frame_rate_hz = 50, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name = "lut4f200", .source = INK_DRV_UC8253_SOURCE_REGISTER, .frames = 4, .frame_rate_hz = 200, .pt_scan = true, .cdi = CDI_PARTIAL},
    {.name          = "3x5f50",
     .source        = INK_DRV_UC8253_SOURCE_REGISTER,
     .frames        = 5,
     .frame_rate_hz = 50,
     .scans         = 3,
     .pt_scan       = true,
     .cdi           = CDI_PARTIAL},
};

static const ink_drv_uc8253_profile_t *profile_at(const ink_drv_uc8253_t *driver, size_t index) {
    return index == TUNE_PROFILE ? &driver->tune_profile : &s_presets[index];
}

static uint32_t scan_count(const ink_drv_uc8253_profile_t *profile) {
    return profile->scans == 0 ? 1 : profile->scans;
}

/* Bảng tần số khung của PLL (UC8253 tr.26). */
static uint32_t pll_hz(uint8_t code) {
    code &= 0x1F;
    if (code < 16) { return 5 + 5U * code; }
    if (code < 24) { return 85 + 5U * (code - 16); }
    return 130 + 10U * (code - 24);
}

/* Mã PLL đúng bằng `hz`; false nếu bảng không có. */
static bool pll_code(uint32_t hz, uint8_t *out_code) {
    for (uint8_t code = 0; code < 32; code++) {
        if (pll_hz(code) == hz) {
            *out_code = code;
            return true;
        }
    }
    return false;
}

static uint32_t typical_ms(const ink_drv_uc8253_profile_t *profile) {
    if (profile->source != INK_DRV_UC8253_SOURCE_REGISTER) { return profile->otp_drf_ms; }
    const uint32_t frames = profile->frames == 0 ? TR8_FRAMES : profile->frames;
    const uint32_t hz     = profile->frame_rate_hz == 0 ? 50 : profile->frame_rate_hz; /* TR8: 50 Hz (waveform.md) */
    return (frames + 2) * 1000 / hz * scan_count(profile);
}

/* Kiểm thông số; với LUT tự dựng thì dựng luôn vào `out_waveform`. */
static esp_err_t check_profile(const ink_drv_uc8253_profile_t *profile, ink_waveform_t *out_waveform) {
    if (profile->source != INK_DRV_UC8253_SOURCE_REGISTER) { return ESP_OK; }
    uint8_t code;
    ESP_RETURN_ON_FALSE(profile->frame_rate_hz == 0 || pll_code(profile->frame_rate_hz, &code), ESP_ERR_INVALID_ARG, TAG,
                        "%u Hz không có trong bảng PLL", profile->frame_rate_hz);
    if (profile->frames == 0) { return ESP_OK; }
    const ink_waveform_parameters_t parameters = {.frames = profile->frames, .hold_frames = profile->hold_frames};
    ESP_RETURN_ON_FALSE(ink_waveform_build(&parameters, out_waveform) == INK_WAVEFORM_OK, ESP_ERR_INVALID_ARG, TAG, "frames %u hold %u",
                        profile->frames, profile->hold_frames);
    return ESP_OK;
}

/* Kiểm cân bằng điện tích (display.md, "Kiểm tra cân bằng điện tích"). */
static esp_err_t check_charge(const char *name, const ink_waveform_t *waveform, bool allow_unbalanced) {
    ink_waveform_charge_t charge;
    if (ink_waveform_charge(waveform, &charge) != INK_WAVEFORM_OK) { return ESP_ERR_INVALID_ARG; }
    if (charge.hold_white != 0 || charge.hold_black != 0) {
        ESP_LOGW(TAG, "%s: bảng giữ màu lệch (trắng %ld, đen %ld khung)", name, (long)charge.hold_white, (long)charge.hold_black);
    }
    if (charge.cycle != 0 && !allow_unbalanced) {
        ESP_LOGE(TAG, "%s: LUTWK + LUTKW = %ld khung, từ chối", name, (long)charge.cycle);
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
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

/* Đọc dải TR8 từ OTP một lần, làm khuôn cho mọi LUT thanh ghi. */
static esp_err_t read_tr8(ink_drv_uc8253_t *driver) {
    if (driver->tr8_valid) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(read_after_command(driver, CMD_ROTP, false, OTP_READ_BYTES), TAG, "rotp");
    const uint8_t *otp = &driver->window_buffer[1];
    /* Chỉ bank 0 đã đọc và kiểm trên module này (waveform.md). */
    ESP_RETURN_ON_FALSE(otp[0] == OTP_LOCK, ESP_ERR_NOT_SUPPORTED, TAG, "OTP bank 0 không có khoá A5");
    memcpy(driver->tr8, &otp[OTP_TR_FIRST + OTP_TR8 * INK_DRV_UC8253_TR_BYTES], INK_DRV_UC8253_TR_BYTES);
    driver->tr8_valid = true;
    return ESP_OK;
}

/* ---- Khởi tạo controller ---- */

static esp_err_t reset_controller(ink_drv_uc8253_t *driver) {
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.reset, 0), TAG, "reset low");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(gpio_set_level(driver->config.reset, 1), TAG, "reset high");
    vTaskDelay(pdMS_TO_TICKS(10)); /* > 1 ms, lệnh gửi sớm hơn bị bỏ qua */
    ESP_RETURN_ON_ERROR(wait_ready(driver, BUSY_SHORT_TIMEOUT_MS), TAG, "after reset");

    /* 0x1E có RST_N = 0 nên là reset mềm: RAM ảnh thành rác (image-memory.md) */
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_PSR, (const uint8_t[]){0x1E, PSR_SECOND}, 2, BUSY_SHORT_TIMEOUT_MS), TAG,
                        "soft reset");
    driver->ram_swapped    = false;
    driver->ram_trusted    = false;
    driver->power_on       = false;
    driver->register_mode  = false;
    driver->loaded_profile = NO_PROFILE;
    driver->needs_reset    = false;
    return send_command(driver, CMD_PSR, (const uint8_t[]){PSR_OTP, PSR_SECOND}, 2);
}

/* ---- Nạp bảng sóng ---- */

static esp_err_t set_register_mode(ink_drv_uc8253_t *driver, bool enable) {
    if (driver->register_mode == enable) { return ESP_OK; }
    const uint8_t psr[2] = {enable ? PSR_REGISTER : PSR_OTP, PSR_SECOND};
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PSR, psr, sizeof(psr)), TAG, "psr");
    driver->register_mode = enable;
    return ESP_OK;
}

/* Gửi điện áp, VCOM, PLL và LUTOPT của TR8, rồi 5 bảng (đã chạy trên BB0, nhánh exp/uc8253-lut). */
static esp_err_t send_register_waveform(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile, const ink_waveform_t *waveform) {
    const uint8_t *tr8       = driver->tr8;
    const uint8_t  pwr[5]    = {0x03, tr8[TR_VG], tr8[TR_VSH], tr8[TR_VSL], tr8[TR_VDHR]};
    const uint8_t  lutopt[5] = {tr8[TR_EOPT], tr8[TR_XON_LOW], tr8[TR_XON_HIGH], 0xFF, 0x00};
    uint8_t        pll       = tr8[TR_FRAME_RATE];
    if (profile->frame_rate_hz != 0) { pll_code(profile->frame_rate_hz, &pll); }
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PWR, pwr, sizeof(pwr)), TAG, "pwr");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_VDCS, &tr8[TR_VCOM], 1), TAG, "vdcs");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_PLL, &pll, 1), TAG, "pll");
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_LUTOPT, lutopt, sizeof(lutopt)), TAG, "lutopt");
    ESP_RETURN_ON_ERROR(wait_ready(driver, BUSY_SHORT_TIMEOUT_MS), TAG, "lutopt");
    for (size_t table = 0; table < INK_WAVEFORM_TABLE_COUNT; table++) {
        static uint8_t bytes[LUT_SEND_BYTES]; /* DMA đọc: không để trên stack */
        memset(bytes, 0, sizeof(bytes));
        memcpy(bytes, waveform->tables[table], INK_WAVEFORM_TABLE_BYTES);
        const size_t length = table == INK_WAVEFORM_TABLE_WHITE_TO_WHITE ? INK_WAVEFORM_TABLE_BYTES : LUT_SEND_BYTES;
        ESP_RETURN_ON_ERROR(send_command(driver, (uint8_t)(CMD_LUTC + table), bytes, length), TAG, "lut %u", (unsigned)table);
    }
    return set_register_mode(driver, true);
}

static esp_err_t load_profile(ink_drv_uc8253_t *driver, size_t index) {
    if (driver->loaded_profile == (int)index) { return ESP_OK; }
    const ink_drv_uc8253_profile_t *profile = profile_at(driver, index);
    if (profile->source != INK_DRV_UC8253_SOURCE_REGISTER) {
        ESP_RETURN_ON_ERROR(set_register_mode(driver, false), TAG, "otp");
    } else {
        ESP_RETURN_ON_ERROR(read_tr8(driver), TAG, "tr8");
        ink_waveform_t waveform;
        if (profile->frames == 0) {
            memcpy(waveform.tables, &driver->tr8[TR_LUT_FIRST], sizeof(waveform.tables));
        } else {
            ESP_RETURN_ON_ERROR(check_profile(profile, &waveform), TAG, "profile");
        }
        ESP_RETURN_ON_ERROR(check_charge(profile->name, &waveform, profile->allow_unbalanced), TAG, "charge");
        ESP_RETURN_ON_ERROR(send_register_waveform(driver, profile, &waveform), TAG, "lut");
    }
    driver->loaded_profile = (int)index;
    return ESP_OK;
}

/* ---- Vẽ ---- */

typedef struct {
    uint16_t x0, x1, y0, y1; /* gồm cả mép; x0, x1 + 1 là bội của X_ALIGN */
} window_t;

/* Vào chế độ partial với window đã căn mép. Bên gọi gửi PTOUT khi xong. */
static esp_err_t enter_window(ink_drv_uc8253_t *driver, const window_t *window, bool pt_scan) {
    const uint8_t ptl[7] = {(uint8_t)window->x0, (uint8_t)window->x1, window->y0 >> 8,      window->y0 & 0xFF,
                            window->y1 >> 8,     window->y1 & 0xFF,   pt_scan ? 0x01 : 0x00};
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

static esp_err_t power_off(ink_drv_uc8253_t *driver) {
    if (!driver->power_on) { return ESP_OK; }
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_POF, nullptr, 0, BUSY_SHORT_TIMEOUT_MS), TAG, "pof");
    driver->power_on = false;
    return ESP_OK;
}

/* Một lần DRF. Full: cả màn, không vào chế độ partial. */
static esp_err_t refresh(ink_drv_uc8253_t *driver, const ink_drv_uc8253_profile_t *profile, const window_t *window, bool full) {
    if (!full) { ESP_RETURN_ON_ERROR(enter_window(driver, window, profile->pt_scan), TAG, "window"); }
    if (profile->source == INK_DRV_UC8253_SOURCE_OTP_FORCED) {
        ESP_RETURN_ON_ERROR(send_command(driver, CMD_CCSET, (const uint8_t[]){0x02}, 1), TAG, "ccset");
        ESP_RETURN_ON_ERROR(send_command(driver, CMD_TSSET, &profile->temperature, 1), TAG, "tsset");
    } else if (profile->source == INK_DRV_UC8253_SOURCE_OTP_SENSOR) {
        /* TSFIX còn từ lần vẽ trước: phải xoá để controller đo nhiệt độ thật */
        ESP_RETURN_ON_ERROR(send_command(driver, CMD_CCSET, (const uint8_t[]){0x00}, 1), TAG, "ccset");
    }
    ESP_RETURN_ON_ERROR(send_command(driver, CMD_CDI, &profile->cdi, 1), TAG, "cdi");
    ESP_RETURN_ON_ERROR(power_on(driver), TAG, "pon");
    ESP_RETURN_ON_ERROR(run_busy_command(driver, CMD_DRF, nullptr, 0, BUSY_REFRESH_TIMEOUT_MS), TAG, "drf");
    /* Sau mỗi DRF controller đổi vai hai vùng RAM (image-memory.md) */
    driver->ram_swapped = !driver->ram_swapped;
    return full ? ESP_OK : send_command(driver, CMD_PTOUT, nullptr, 0);
}

static uint32_t elapsed_us(int64_t *since_us) {
    const int64_t now_us = esp_timer_get_time();
    const int64_t result = now_us - *since_us;
    *since_us            = now_us;
    return (uint32_t)result;
}

static void apply_tune(ink_drv_uc8253_t *driver) {
    taskENTER_CRITICAL(&driver->tune_lock);
    const bool                     pending = driver->tune_pending;
    const ink_hal_display_slot_t   slot    = driver->tune_slot;
    const ink_drv_uc8253_profile_t profile = driver->tune_request;
    driver->tune_pending                   = false;
    taskEXIT_CRITICAL(&driver->tune_lock);
    if (!pending) { return; }
    driver->tune_profile                          = profile;
    driver->tune_profile.name                     = driver->profile_info[TUNE_PROFILE].name;
    driver->profile_info[TUNE_PROFILE].typical_ms = typical_ms(&driver->tune_profile);
    driver->slot_profiles[slot]                   = TUNE_PROFILE;
    if (driver->loaded_profile == TUNE_PROFILE) { driver->loaded_profile = NO_PROFILE; }
}

/* Window đã căn mép, hoặc cả màn khi full hay khi RAM không còn đáng tin. */
static esp_err_t make_window(const ink_drv_uc8253_t *driver, ink_hal_display_rect_t rect, bool full, window_t *out_window) {
    if (full || !driver->ram_trusted) {
        *out_window = (window_t){.x0 = 0, .x1 = INK_DRV_UC8253_WIDTH - 1, .y0 = 0, .y1 = INK_DRV_UC8253_HEIGHT - 1};
        return ESP_OK;
    }
    ESP_RETURN_ON_FALSE(
        rect.width > 0 && rect.height > 0 && rect.x + rect.width <= INK_DRV_UC8253_WIDTH && rect.y + rect.height <= INK_DRV_UC8253_HEIGHT,
        ESP_ERR_INVALID_ARG, TAG, "window");
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
                             ink_hal_display_slot_t slot, ink_hal_display_timing_t *out_timing) {
    if (driver->needs_reset) { ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset"); }
    apply_tune(driver);
    const bool full = slot == INK_HAL_DISPLAY_SLOT_FULL;
    window_t   window;
    ESP_RETURN_ON_ERROR(make_window(driver, rect, full, &window), TAG, "window");
    const size_t                    index   = driver->slot_profiles[slot];
    const ink_drv_uc8253_profile_t *profile = profile_at(driver, index);

    int64_t mark_us = esp_timer_get_time();
    *out_timing     = (ink_hal_display_timing_t){};
    ESP_RETURN_ON_ERROR(load_profile(driver, index), TAG, "profile %s", profile->name);
    out_timing->spi_us += elapsed_us(&mark_us);

    driver->needs_reset = true; /* lỗi giữa chừng thì không còn biết lượt */
    for (uint32_t scan = 0; scan < scan_count(profile); scan++) {
        /* Số DRF từ reset chẵn: mới vào 0x13, cũ vào 0x10; lẻ thì ngược lại. Ghi cả
         * hai trước DRF như luồng của hãng (GDEY037T03 tr.39). */
        const uint8_t new_command = driver->ram_swapped ? CMD_DTM1 : CMD_DTM2;
        const uint8_t old_command = driver->ram_swapped ? CMD_DTM2 : CMD_DTM1;
        ESP_RETURN_ON_ERROR(write_plane(driver, new_command, plane_new, &window), TAG, "new plane");
        ESP_RETURN_ON_ERROR(write_plane(driver, old_command, plane_old, &window), TAG, "old plane");
        out_timing->spi_us += elapsed_us(&mark_us);
        ESP_RETURN_ON_ERROR(refresh(driver, profile, &window, full), TAG, "refresh");
        out_timing->panel_us += elapsed_us(&mark_us);
    }
    driver->needs_reset = false;
    driver->ram_trusted = true;

    if (full || !profile->keep_power_on) { ESP_RETURN_ON_ERROR(power_off(driver), TAG, "pof"); }
    out_timing->after_us = elapsed_us(&mark_us);
    return ESP_OK;
}

/* ---- Bảng hàm ink_hal_display ---- */

static esp_err_t draw(void *context, const uint8_t *plane_new, const uint8_t *plane_old, ink_hal_display_rect_t window,
                      ink_hal_display_slot_t slot, ink_hal_display_timing_t *out_timing) {
    ESP_RETURN_ON_FALSE(context && plane_new && plane_old && out_timing && slot < INK_HAL_DISPLAY_SLOT_COUNT, ESP_ERR_INVALID_ARG, TAG,
                        "arg");
    return draw_planes(context, plane_new, plane_old, window, slot, out_timing);
}

static esp_err_t select_profile(void *context, ink_hal_display_slot_t slot, size_t index) {
    ink_drv_uc8253_t *driver = context;
    ESP_RETURN_ON_FALSE(driver && slot < INK_HAL_DISPLAY_SLOT_COUNT && index < INK_DRV_UC8253_PROFILE_COUNT, ESP_ERR_INVALID_ARG, TAG,
                        "arg");
    driver->slot_profiles[slot] = index;
    return ESP_OK;
}

/* Lệnh 0x40 kéo BUSY xuống trong lúc đo; byte đầu là TS[7:0], °C có dấu (UC8253 tr.27).
 * Chưa chạy trên BB0. */
static esp_err_t read_temperature(void *context, int8_t *out_celsius) {
    ink_drv_uc8253_t *driver = context;
    ESP_RETURN_ON_FALSE(driver && out_celsius, ESP_ERR_INVALID_ARG, TAG, "arg");
    ESP_RETURN_ON_ERROR(read_after_command(driver, CMD_TSC, true, 2), TAG, "tsc");
    *out_celsius = (int8_t)driver->window_buffer[0];
    return ESP_OK;
}

static const ink_hal_display_operations_t s_operations = {
    .draw             = draw,
    .select_profile   = select_profile,
    .read_temperature = read_temperature,
};

/* ---- Chỉnh tay ---- */

esp_err_t ink_drv_uc8253_tune(ink_drv_uc8253_t *driver, ink_hal_display_slot_t slot, const ink_drv_uc8253_profile_t *profile) {
    ESP_RETURN_ON_FALSE(driver && profile && slot < INK_HAL_DISPLAY_SLOT_COUNT, ESP_ERR_INVALID_ARG, TAG, "arg");
    ink_waveform_t waveform;
    ESP_RETURN_ON_ERROR(check_profile(profile, &waveform), TAG, "profile");
    if (profile->source == INK_DRV_UC8253_SOURCE_REGISTER && profile->frames != 0) {
        ESP_RETURN_ON_ERROR(check_charge("tune", &waveform, false), TAG, "charge");
    }
    taskENTER_CRITICAL(&driver->tune_lock);
    driver->tune_request = *profile;
    driver->tune_slot    = slot;
    driver->tune_pending = true;
    taskEXIT_CRITICAL(&driver->tune_lock);
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

esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config, ink_hal_display_device_t *out_device) {
    ESP_RETURN_ON_FALSE(driver && config && out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    *driver = (ink_drv_uc8253_t){
        .config         = *config,
        .slot_profiles  = {[INK_HAL_DISPLAY_SLOT_FULL] = DEFAULT_FULL, [INK_HAL_DISPLAY_SLOT_PARTIAL] = DEFAULT_PARTIAL},
        .tune_profile   = s_presets[DEFAULT_PARTIAL],
        .loaded_profile = NO_PROFILE,
        .tune_lock      = portMUX_INITIALIZER_UNLOCKED,
    };
    for (size_t i = 0; i < TUNE_PROFILE; i++) {
        driver->profile_info[i] = (ink_hal_display_profile_info_t){.name = s_presets[i].name, .typical_ms = typical_ms(&s_presets[i])};
    }
    driver->profile_info[TUNE_PROFILE] = (ink_hal_display_profile_info_t){.name = "tune", .typical_ms = typical_ms(&driver->tune_profile)};
    driver->tune_profile.name          = "tune";

    ESP_RETURN_ON_ERROR(configure_pins(config), TAG, "pins");
    ESP_RETURN_ON_ERROR(add_device(driver, false), TAG, "spi_bus_add_device");
    driver->busy_ready = xSemaphoreCreateBinaryStatic(&driver->busy_ready_storage);
    /* Dịch vụ ngắt GPIO do bsp cài (ink_bsp_init). */
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(config->busy, on_busy_rise, driver), TAG, "gpio_isr_handler_add");
    ESP_RETURN_ON_ERROR(reset_controller(driver), TAG, "reset");

    *out_device = (ink_hal_display_device_t){
        .operations = &s_operations,
        .context    = driver,
        .capabilities =
            {
                           .width            = INK_DRV_UC8253_WIDTH,
                           .height           = INK_DRV_UC8253_HEIGHT,
                           .x_align          = X_ALIGN,
                           .profiles         = driver->profile_info,
                           .profile_count    = INK_DRV_UC8253_PROFILE_COUNT,
                           .default_profiles = {[INK_HAL_DISPLAY_SLOT_FULL] = DEFAULT_FULL, [INK_HAL_DISPLAY_SLOT_PARTIAL] = DEFAULT_PARTIAL},
                           },
    };
    return ESP_OK;
}
