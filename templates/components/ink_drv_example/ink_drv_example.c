/* Thiết kế: docs/design/hardware-variants.md */
#include "ink_drv_example.h"

#include "esp_check.h"

static const char *TAG = "ink_drv_example";

static esp_err_t read_level(void *ctx, bool *out_active)
{
    const ink_drv_example_t *drv  = ctx;
    const bool               high = gpio_get_level(drv->cfg.pin) != 0;
    *out_active                   = drv->cfg.active_low ? !high : high;
    return ESP_OK;
}

static const ink_hal_example_ops_t s_ops = {
    .read = read_level,
};

esp_err_t ink_drv_example_init(ink_drv_example_t *drv, const ink_drv_example_config_t *cfg,
                               ink_hal_example_dev_t *out_dev)
{
    ESP_RETURN_ON_FALSE(drv && cfg && out_dev, ESP_ERR_INVALID_ARG, TAG, "null arg");
    const gpio_config_t io = {
        .pin_bit_mask = 1ULL << cfg->pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = cfg->active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = cfg->active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config pin %d", cfg->pin);
    drv->cfg = *cfg;
    *out_dev = (ink_hal_example_dev_t){
        .ops  = &s_ops,
        .ctx  = drv,
        .caps = {.has_irq = false},
    };
    return ESP_OK;
}
