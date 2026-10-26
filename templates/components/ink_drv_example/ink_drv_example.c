/* Thiết kế: docs/design/hardware-variants.md */
#include "ink_drv_example.h"

#include "esp_check.h"

static const char *TAG = "ink_drv_example";

/* ---- Bảng hàm ink_hal_example ---- */

static esp_err_t read_active(void *context, bool *out_active)
{
    const ink_drv_example_t *driver     = context;
    const bool               level_high = gpio_get_level(driver->config.pin) != 0;
    *out_active                         = driver->config.active_low ? !level_high : level_high;
    return ESP_OK;
}

static const ink_hal_example_operations_t s_operations = {
    .read = read_active,
};

/* ---- Khởi tạo ---- */

esp_err_t ink_drv_example_init(ink_drv_example_t *driver, const ink_drv_example_config_t *config,
                               ink_hal_example_device_t *out_device)
{
    ESP_RETURN_ON_FALSE(driver && config && out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    const gpio_config_t pin_config = {
        .pin_bit_mask = 1ULL << config->pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = config->active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = config->active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&pin_config), TAG, "gpio_config pin %d", config->pin);
    driver->config = *config;

    *out_device = (ink_hal_example_device_t){
        .operations   = &s_operations,
        .context      = driver,
        .capabilities = {.has_interrupt = false},
    };
    return ESP_OK;
}
