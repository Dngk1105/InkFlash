#include <string.h>
#include "driver/spi_master.h"
#include "drv_display_chip.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_log.h"

/*
 * Driver màn Good Display GDEY037T03 (module WeAct 3.7"): 240×416, controller UC8253, đen/trắng.
 * Chuỗi lệnh chuyển thể từ thư viện GxEPD2 (GxEPD2_370_GDEY037T03.cpp), chưa đối chiếu với datasheet UC8253.
 *
 * Hai kiểu làm mới toàn màn: FULL (sóng chuẩn ~3 s) và CLEAN (đảo đen/trắng trước rồi vẽ, ~9 s với 1 vòng).
 *
 * Bộ nhớ ảnh của controller có hai vùng: 0x13 = ảnh mới, 0x10 = ảnh cũ. Làm mới nhanh (partial) so sánh hai vùng nên
 * sau mỗi lần làm mới phải ghi lại ảnh cũ bằng ảnh mới (write_image 0x10).
 */
#define NATIVE_W       240
#define NATIVE_H       416
#define NATIVE_BYTES   (NATIVE_W * NATIVE_H / 8)
#define SPI_HOST_ID    SPI2_HOST
#define SPI_CLOCK_HZ   4000000 /* như GxEPD2 */
#define SPI_CHUNK      4096
#define RESET_MS       50      /* như demo: display.init(.., reset_duration = 50) */
#define BUSY_TIMEOUT_MS 10000

#define CHECK(x)                          \
    do {                                  \
        esp_err_t e_ = (x);               \
        if (e_ != ESP_OK) {               \
            return e_;                    \
        }                                 \
    } while (0)

static drv_display_cfg_t s_cfg;
static spi_device_handle_t s_spi;
static uint8_t *s_buf; /* vùng ảnh native (đã xoay) chuẩn bị gửi, DMA-capable */
static bool s_bus_ready;
static bool s_init_display_done;
static bool s_power_on;
static bool s_initial = true; /* chưa vẽ lần nào (hoặc vừa ngủ sâu): phải xoá trắng toàn màn trước */
static bool s_hibernating;

/* ---- Tầng SPI + chân điều khiển ---- */

static void dc_pre_cb(spi_transaction_t *t)
{
    gpio_set_level(s_cfg.dc, (int)(intptr_t)t->user);
}

static esp_err_t tx(const uint8_t *data, size_t len, int dc)
{
    while (len) {
        size_t n = len > SPI_CHUNK ? SPI_CHUNK : len;
        spi_transaction_t t = {
            .length = n * 8,
            .tx_buffer = data,
            .user = (void *)(intptr_t)dc,
        };
        CHECK(spi_device_polling_transmit(s_spi, &t));
        data += n;
        len -= n;
    }
    return ESP_OK;
}

static esp_err_t cmd(uint8_t c)
{
    return tx(&c, 1, 0);
}

static esp_err_t data(const uint8_t *d, size_t n)
{
    return tx(d, n, 1);
}

static esp_err_t data1(uint8_t d)
{
    return tx(&d, 1, 1);
}

static void delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms) ? pdMS_TO_TICKS(ms) : 1);
}

static void hw_reset(void)
{
    gpio_set_level(s_cfg.rst, 1);
    delay_ms(10);
    gpio_set_level(s_cfg.rst, 0);
    delay_ms(RESET_MS);
    gpio_set_level(s_cfg.rst, 1);
    delay_ms(RESET_MS > 10 ? RESET_MS : 10);
    s_hibernating = false;
}

/* BUSY mức thấp = đang bận (theo GxEPD2: busy_level LOW). */
static esp_err_t wait_busy(const char *what)
{
    delay_ms(1);
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(s_cfg.busy) == 0) {
        if ((esp_timer_get_time() - start) / 1000 > BUSY_TIMEOUT_MS) {
            INK_LOGE(INK_COMP_EPD, INK_LOG_EVT_STATE, "BUSY timeout: %s", what);
            return ESP_ERR_TIMEOUT;
        }
        delay_ms(1);
    }
    return ESP_OK;
}

/* ---- Lệnh UC8253 ---- */

static esp_err_t power_on(void)
{
    if (!s_power_on) {
        CHECK(cmd(0x04));
        CHECK(wait_busy("power on"));
    }
    s_power_on = true;
    return ESP_OK;
}

static esp_err_t power_off(void)
{
    if (s_power_on) {
        CHECK(cmd(0x02));
        CHECK(wait_busy("power off"));
    }
    s_power_on = false;
    return ESP_OK;
}

static esp_err_t init_display(void)
{
    if (s_hibernating) {
        hw_reset();
    } else {
        const uint8_t soft_reset[] = { 0x1e, 0x0d };
        CHECK(cmd(0x00)); /* PANEL SETTING: soft reset */
        CHECK(data(soft_reset, sizeof(soft_reset)));
        delay_ms(1);
    }
    s_power_on = false;
    const uint8_t panel[] = { 0x1f, 0x0d }; /* BWOTP */
    CHECK(cmd(0x00));
    CHECK(data(panel, sizeof(panel)));
    s_init_display_done = true;
    return ESP_OK;
}

typedef struct {
    uint16_t x, y, w, h; /* native, x và w là bội số của 8 */
} nrect_t;

static esp_err_t set_ram_area(const nrect_t *r)
{
    uint16_t xe = (r->x + r->w - 1) | 0x0007;
    uint16_t ye = r->y + r->h - 1;
    const uint8_t win[] = { (uint8_t)r->x, (uint8_t)xe, (uint8_t)(r->y >> 8), (uint8_t)(r->y & 0xff),
                            (uint8_t)(ye >> 8), (uint8_t)(ye & 0xff), 0x01 };
    CHECK(cmd(0x90)); /* partial window */
    return data(win, sizeof(win));
}

/* Ghi s_buf (đã pack đúng nrect) vào vùng nhớ `target`: 0x13 = ảnh mới, 0x10 = ảnh cũ. */
static esp_err_t write_image(uint8_t target, const nrect_t *r)
{
    CHECK(cmd(0x91)); /* partial in */
    CHECK(set_ram_area(r));
    CHECK(cmd(target));
    CHECK(data(s_buf, (size_t)(r->w / 8) * r->h));
    return cmd(0x92); /* partial out */
}

/*
 * Làm mới toàn phần. Mặc định dùng sóng chuẩn (~3 s, sạch hơn). Bật INK_DISPLAY_UC8253_FAST_FULL thì ép nhiệt độ
 * (TSFIX) để chỉ mất ~1 s nhưng ảnh hơi xám và dễ để lại bóng; GxEPD2 gọi đó là useFastFullUpdate.
 */
static esp_err_t update_full(void)
{
#if CONFIG_INK_DISPLAY_UC8253_FAST_FULL
    const uint8_t tsfix = 0x02;
    CHECK(cmd(0xE0)); /* cascade setting */
    CHECK(data(&tsfix, 1));
    CHECK(cmd(0xE5)); /* force temperature */
    CHECK(data1(0x5A));
#endif
    CHECK(cmd(0x50)); /* VCOM and data interval */
    CHECK(data1(0x97));
    CHECK(power_on());
    CHECK(cmd(0x12)); /* display refresh */
    CHECK(wait_busy("full refresh"));
    CHECK(power_off());
#if CONFIG_INK_DISPLAY_UC8253_FAST_FULL
    return init_display(); /* huỷ TSFIX */
#else
    return ESP_OK;
#endif
}

static esp_err_t update_part(void)
{
    CHECK(cmd(0xE0));
    CHECK(data1(0x02));
    CHECK(cmd(0xE5));
    CHECK(data1(0x6E)); /* nhiệt độ ép để làm mới nhanh */
    CHECK(cmd(0x50));
    CHECK(data1(0xD7));
    CHECK(power_on());
    CHECK(cmd(0x12));
    CHECK(wait_busy("partial refresh"));
    CHECK(power_off());
    return init_display();
}

static esp_err_t refresh_rect(const nrect_t *r)
{
    CHECK(cmd(0x91));
    CHECK(set_ram_area(r));
    CHECK(update_part());
    return cmd(0x92);
}

/* ---- Tọa độ: logic (đã xoay) <-> native 240×416 ---- */

static void to_native(int lx, int ly, int *nx, int *ny)
{
    switch (s_cfg.rotation & 3) {
    case 1: *nx = NATIVE_W - 1 - ly; *ny = lx; break;
    case 2: *nx = NATIVE_W - 1 - lx; *ny = NATIVE_H - 1 - ly; break;
    case 3: *nx = ly; *ny = NATIVE_H - 1 - lx; break;
    default: *nx = lx; *ny = ly; break;
    }
}

static void to_logical(int nx, int ny, int *lx, int *ly)
{
    switch (s_cfg.rotation & 3) {
    case 1: *lx = ny; *ly = NATIVE_W - 1 - nx; break;
    case 2: *lx = NATIVE_W - 1 - nx; *ly = NATIVE_H - 1 - ny; break;
    case 3: *lx = NATIVE_H - 1 - ny; *ly = nx; break;
    default: *lx = nx; *ly = ny; break;
    }
}

static uint16_t logical_width(void)
{
    return (s_cfg.rotation & 1) ? NATIVE_H : NATIVE_W;
}

/* Vùng logic → vùng native, mở rộng x ra bội số của 8 (bộ nhớ controller chia theo byte theo trục x native). */
static nrect_t native_rect(const ink_rect_t *region)
{
    if (!region) {
        return (nrect_t){ 0, 0, NATIVE_W, NATIVE_H };
    }
    int ax, ay, bx, by;
    to_native(region->x, region->y, &ax, &ay);
    to_native(region->x + region->w - 1, region->y + region->h - 1, &bx, &by);
    int x0 = (ax < bx ? ax : bx) & ~7;
    int x1 = (ax > bx ? ax : bx) | 7;
    int y0 = ay < by ? ay : by;
    int y1 = ay > by ? ay : by;
    return (nrect_t){ x0, y0, x1 - x0 + 1, y1 - y0 + 1 };
}

/* Chép vùng native r từ framebuffer logic (1 = trắng, MSB trước) vào s_buf. */
static void pack_region(const uint8_t *fb, const nrect_t *r)
{
    const int lstride = (logical_width() + 7) / 8;
    uint8_t *p = s_buf;
    for (int ny = r->y; ny < r->y + r->h; ny++) {
        for (int j = 0; j < r->w / 8; j++) {
            uint8_t byte = 0;
            for (int b = 0; b < 8; b++) {
                int lx, ly;
                to_logical(r->x + j * 8 + b, ny, &lx, &ly);
                if (fb[ly * lstride + (lx >> 3)] & (0x80 >> (lx & 7))) {
                    byte |= 0x80 >> b;
                }
            }
            *p++ = byte;
        }
    }
}

/* ---- Giao diện cho HAL ---- */

static esp_err_t uc_get_info(void *ctx, ink_display_info_t *out)
{
    const bool landscape = s_cfg.rotation & 1;
    out->width = landscape ? NATIVE_H : NATIVE_W;
    out->height = landscape ? NATIVE_W : NATIVE_H;
    out->partial_refresh = true;
    out->deep_clean = true;
    return ESP_OK;
}

/* Ghi một màu phẳng (0x00 = đen, 0xFF = trắng) vào cả hai vùng nhớ ảnh rồi làm mới toàn phần. */
static esp_err_t flood_refresh(uint8_t value)
{
    const nrect_t full = { 0, 0, NATIVE_W, NATIVE_H };
    memset(s_buf, value, NATIVE_BYTES);
    CHECK(write_image(0x10, &full));
    CHECK(write_image(0x13, &full));
    return update_full();
}

/*
 * Xoá sạch: mỗi vòng cả màn đen rồi cả màn trắng. Đảo hết hạt mực giúp xoá bóng mờ và chấm lốm đốm.
 * `cycles` = 0 thì chỉ xoá trắng một lần. Luôn kết thúc bằng màn trắng.
 */
static esp_err_t deep_clean(int cycles)
{
    for (int i = 0; i < cycles; i++) {
        CHECK(flood_refresh(0x00));
        CHECK(flood_refresh(0xFF));
    }
    if (cycles == 0) {
        CHECK(flood_refresh(0xFF));
    }
    return ESP_OK;
}

static esp_err_t clear_screen(void)
{
    CHECK(deep_clean(CONFIG_INK_DISPLAY_UC8253_CLEAN_CYCLES));
    s_initial = false;
    return ESP_OK;
}

static esp_err_t uc_draw(void *ctx, const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode)
{
    if (!s_init_display_done) {
        CHECK(init_display());
    }
    if (s_initial) {
        CHECK(clear_screen()); /* lần vẽ đầu: xoá sạch để ảnh cũ/mới khớp nhau và không còn hạt mực cũ */
    }
    if (mode == INK_REFRESH_CLEAN) {
        int cycles = CONFIG_INK_DISPLAY_UC8253_CLEAN_CYCLES;
        CHECK(deep_clean(cycles > 0 ? cycles : 1));
    }
    const bool full = (mode != INK_REFRESH_PARTIAL) || !region;
    const nrect_t r = native_rect(full ? NULL : region);
    pack_region(fb, &r);
    if (full) {
        CHECK(write_image(0x10, &r));
        CHECK(write_image(0x13, &r));
        CHECK(update_full());
        CHECK(write_image(0x10, &r)); /* ảnh cũ = ảnh mới, để lần partial sau so sánh đúng */
    } else {
        CHECK(write_image(0x13, &r));
        CHECK(refresh_rect(&r));
        CHECK(write_image(0x10, &r));
    }
    return ESP_OK;
}

static esp_err_t uc_sleep(void *ctx)
{
    if (!s_init_display_done) {
        return ESP_OK; /* chưa vẽ gì, panel chưa được đánh thức */
    }
    CHECK(power_off());
    CHECK(cmd(0x07)); /* deep sleep */
    CHECK(data1(0xA5)); /* mã kiểm tra */
    s_hibernating = true;
    s_init_display_done = false;
    s_initial = true; /* bộ nhớ ảnh của controller mất khi ngủ sâu */
    return ESP_OK;
}

static esp_err_t uc_init(const drv_display_cfg_t *cfg)
{
    if (cfg->sck == GPIO_NUM_NC || cfg->mosi == GPIO_NUM_NC || cfg->cs == GPIO_NUM_NC || cfg->dc == GPIO_NUM_NC ||
        cfg->rst == GPIO_NUM_NC || cfg->busy == GPIO_NUM_NC) {
        INK_LOGE(INK_COMP_EPD, INK_LOG_EVT_SELF_TEST_FAIL, "UC8253 can du 6 chan: sck mosi cs dc rst busy");
        return ESP_ERR_INVALID_STATE;
    }
    s_cfg = *cfg;

    const gpio_config_t out = {
        .pin_bit_mask = (1ULL << cfg->dc) | (1ULL << cfg->rst),
        .mode = GPIO_MODE_OUTPUT,
    };
    CHECK(gpio_config(&out));
    const gpio_config_t in = {
        .pin_bit_mask = 1ULL << cfg->busy,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, /* như demo: INPUT_PULLUP */
    };
    CHECK(gpio_config(&in));

    if (!s_bus_ready) {
        const spi_bus_config_t bus = {
            .mosi_io_num = cfg->mosi,
            .miso_io_num = -1,
            .sclk_io_num = cfg->sck,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SPI_CHUNK,
        };
        CHECK(spi_bus_initialize(SPI_HOST_ID, &bus, SPI_DMA_CH_AUTO));
        const spi_device_interface_config_t dev = {
            .clock_speed_hz = SPI_CLOCK_HZ,
            .mode = 0,
            .spics_io_num = cfg->cs,
            .queue_size = 1,
            .pre_cb = dc_pre_cb,
        };
        CHECK(spi_bus_add_device(SPI_HOST_ID, &dev, &s_spi));
        s_buf = heap_caps_malloc(NATIVE_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        if (!s_buf) {
            return ESP_ERR_NO_MEM;
        }
        s_bus_ready = true;
    }
    hw_reset();
    s_init_display_done = false; /* init_display() chạy lười ở lần vẽ đầu */
    return ESP_OK;
}

static const ink_display_ops_t s_ops = {
    .get_info = uc_get_info,
    .draw = uc_draw,
    .sleep = uc_sleep,
};

const drv_display_chip_t display_uc8253_chip = {
    .ops = &s_ops,
    .init = uc_init,
};
