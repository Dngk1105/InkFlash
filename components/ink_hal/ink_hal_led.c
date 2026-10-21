#include "ink_hal_led.h"

static const ink_led_ops_t *s_ops;
static void *s_ctx;

esp_err_t ink_led_attach(const ink_led_ops_t *ops, void *ctx)
{
    if (!ops || !ops->set_rgb) {
        return ESP_ERR_INVALID_ARG;
    }
    s_ctx = ctx;
    s_ops = ops;
    return ESP_OK;
}

bool ink_led_present(void)
{
    return s_ops != NULL;
}

esp_err_t ink_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    return s_ops ? s_ops->set_rgb(s_ctx, r, g, b) : ESP_ERR_NOT_FOUND;
}
