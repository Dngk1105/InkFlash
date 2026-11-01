/* Thiết kế: docs/design/hardware-variants.md */
#include "ink_bsp.h"

#include "esp_check.h"
#include "ink_drv_gpio_button.h"
#include "ink_drv_uc8253.h"
#include "ink_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* ---- Lệnh console epd ---- */

/* epd tune <full|partial> <khung> <Hz> [giữ] [lần quét] [giữ nguồn 0|1]; khung 0 = chép nguyên bảng TR8. */
static int on_epd_command(int argc, char **argv) {
    if (argc < 5 || strcmp(argv[1], "tune") != 0) {
        printf("epd tune <full|partial> <khung> <Hz> [giữ] [lần quét] [giữ nguồn 0|1]\n");
        return 1;
    }
    const ink_hal_display_slot_t   slot    = strcmp(argv[2], "full") == 0 ? INK_HAL_DISPLAY_SLOT_FULL : INK_HAL_DISPLAY_SLOT_PARTIAL;
    const ink_drv_uc8253_profile_t profile = {
        .source        = INK_DRV_UC8253_SOURCE_REGISTER,
        .frames        = (uint8_t)atoi(argv[3]),
        .frame_rate_hz = (uint16_t)atoi(argv[4]),
        .hold_frames   = argc > 5 ? (uint8_t)atoi(argv[5]) : 0,
        .scans         = argc > 6 ? (uint8_t)atoi(argv[6]) : 1,
        .keep_power_on = argc > 7 && atoi(argv[7]) != 0,
        .pt_scan       = true,
        .cdi           = 0xD7, /* viền nổi, như partial của GxEPD2 */
    };
    if (ink_drv_uc8253_tune(&s_display_driver, slot, &profile) != ESP_OK) { return 1; }
    printf("tune: lần vẽ sau dùng profile tune cho slot %s\n", argv[2]);
    return 0;
}

esp_err_t ink_bsp_add_console_command(void) {
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    return ink_platform_console_add("epd", "Chỉnh tay bảng sóng UC8253: epd tune ...",
                                    "tune <full|partial> <khung> <Hz> [giữ] [quét] [nguồn]", on_epd_command);
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
