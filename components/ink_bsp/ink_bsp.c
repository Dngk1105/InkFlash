/* Thiết kế: docs/design/hardware-variants.md */
#include "ink_bsp.h"

#include "esp_check.h"
#include "ink_drv_gpio_button.h"
#include "ink_drv_uc8253.h"

static const char *TAG = "ink_bsp";

static bool s_initialized;

/* ---- Nút ---- */

#define BUTTON_COUNT 2

/* BB0 (docs/hardware/boards/bb0.md). GPIO46 là chân strapping: chỉ dùng kéo lên
 * trong. */
static const ink_drv_gpio_button_config_t s_button_pins[BUTTON_COUNT] = {
    {.pin = GPIO_NUM_8,  .active_low = true, .pull_up = true},
    {.pin = GPIO_NUM_46, .active_low = true, .pull_up = true},
};

static ink_drv_gpio_button_t   s_button_drivers[BUTTON_COUNT];
static ink_hal_button_device_t s_button_devices[BUTTON_COUNT];

static esp_err_t init_buttons(void) {
    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        ESP_RETURN_ON_ERROR(ink_drv_gpio_button_init(&s_button_drivers[i], &s_button_pins[i], &s_button_devices[i]), TAG, "button %u",
                            (unsigned)i);
    }
    return ESP_OK;
}

esp_err_t ink_bsp_buttons(const ink_hal_button_device_t **out_devices, size_t *out_count) {
    ESP_RETURN_ON_FALSE(out_devices && out_count, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    *out_devices = s_button_devices;
    *out_count   = BUTTON_COUNT;
    return ESP_OK;
}

/* ---- Màn ---- */

#define DISPLAY_SPI_HOST SPI2_HOST

/* BB0 (docs/hardware/boards/bb0.md). Màn chỉ ghi, không có MISO. */
static const gpio_num_t s_display_sclk = GPIO_NUM_4;
static const gpio_num_t s_display_mosi = GPIO_NUM_5;

static const ink_drv_uc8253_config_t s_display_pins = {
    .host  = DISPLAY_SPI_HOST,
    .cs    = GPIO_NUM_6,
    .dc    = GPIO_NUM_7,
    .reset = GPIO_NUM_15,
    .busy  = GPIO_NUM_16,
};

static ink_drv_uc8253_t         s_display_driver;
static ink_hal_display_device_t s_display_device;

static esp_err_t init_display(void) {
    const spi_bus_config_t bus_config = {
        .sclk_io_num     = s_display_sclk,
        .mosi_io_num     = s_display_mosi,
        .miso_io_num     = -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = INK_DRV_UC8253_FRAME_BYTES,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(DISPLAY_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO), TAG, "spi_bus_initialize");
    return ink_drv_uc8253_init(&s_display_driver, &s_display_pins, &s_display_device);
}

esp_err_t ink_bsp_display(const ink_hal_display_device_t **out_device) {
    ESP_RETURN_ON_FALSE(out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    *out_device = &s_display_device;
    return ESP_OK;
}

esp_err_t ink_bsp_display_load_profile(const uint8_t *blob, size_t length) {
    ESP_RETURN_ON_FALSE(blob != nullptr, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    return ink_drv_uc8253_load_temporary(&s_display_driver, blob, length);
}

esp_err_t ink_bsp_display_read_otp(uint8_t *out, size_t capacity, size_t *out_length) {
    ESP_RETURN_ON_FALSE(out && out_length, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    ESP_RETURN_ON_ERROR(ink_drv_uc8253_read_otp(&s_display_driver, out, capacity), TAG, "otp");
    *out_length = INK_DRV_UC8253_OTP_BYTES;
    return ESP_OK;
}

esp_err_t ink_bsp_display_set_psr_second(uint8_t value) {
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    return ink_drv_uc8253_set_psr_second(&s_display_driver, value);
}

uint8_t ink_bsp_display_psr_second(void) {
    return ink_drv_uc8253_psr_second(&s_display_driver);
}

esp_err_t ink_bsp_display_set_vcom_override(uint8_t value) {
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    return ink_drv_uc8253_set_vcom_override(&s_display_driver, value);
}

uint8_t ink_bsp_display_vcom_override(void) {
    return ink_drv_uc8253_vcom_override(&s_display_driver);
}

/* ---- Khởi tạo board ---- */

esp_err_t ink_bsp_init(void) {
    ESP_RETURN_ON_FALSE(!s_initialized, ESP_ERR_INVALID_STATE, TAG, "already init");
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "gpio_install_isr_service");
    ESP_RETURN_ON_ERROR(init_buttons(), TAG, "buttons");
    ESP_RETURN_ON_ERROR(init_display(), TAG, "display");
    s_initialized = true;
    ESP_LOGI(TAG, "board BB0");
    return ESP_OK;
}
