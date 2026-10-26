/* Thiết kế: docs/design/hardware-variants.md */
#include "ink_bsp.h"

#include "esp_check.h"
#include "ink_drv_gpio_button.h"

static const char *TAG = "ink_bsp";

static bool s_initialized;

/* ---- Nút ---- */

#define BUTTON_COUNT 2

/* BB0 (docs/hardware/boards/bb0.md). GPIO46 là chân strapping: chỉ dùng kéo lên trong. */
static const ink_drv_gpio_button_config_t s_button_pins[BUTTON_COUNT] = {
    {.pin = GPIO_NUM_8,  .active_low = true, .pull_up = true},
    {.pin = GPIO_NUM_46, .active_low = true, .pull_up = true},
};

static ink_drv_gpio_button_t   s_button_drivers[BUTTON_COUNT];
static ink_hal_button_device_t s_button_devices[BUTTON_COUNT];

static esp_err_t init_buttons(void)
{
    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        ESP_RETURN_ON_ERROR(ink_drv_gpio_button_init(&s_button_drivers[i], &s_button_pins[i], &s_button_devices[i]),
                            TAG, "button %u", (unsigned)i);
    }
    return ESP_OK;
}

esp_err_t ink_bsp_buttons(const ink_hal_button_device_t **out_devices, size_t *out_count)
{
    ESP_RETURN_ON_FALSE(out_devices && out_count, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not init");
    *out_devices = s_button_devices;
    *out_count   = BUTTON_COUNT;
    return ESP_OK;
}

/* ---- Khởi tạo board ---- */

esp_err_t ink_bsp_init(void)
{
    ESP_RETURN_ON_FALSE(!s_initialized, ESP_ERR_INVALID_STATE, TAG, "already init");
    ESP_RETURN_ON_ERROR(init_buttons(), TAG, "buttons");
    s_initialized = true;
    ESP_LOGI(TAG, "board BB0");
    return ESP_OK;
}
