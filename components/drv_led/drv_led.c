#include "drv_led.h"
#include "ink_device.h"
#include "ink_hal_led.h"
#include "sdkconfig.h"

#if CONFIG_INK_LED_WS2812
extern esp_err_t led_ws2812_init(const drv_led_cfg_t *cfg);
extern const ink_led_ops_t led_ws2812_ops;

static drv_led_cfg_t s_cfg;

static esp_err_t led_init(void *ctx)
{
    esp_err_t err = led_ws2812_init(&s_cfg);
    if (err != ESP_OK) {
        return err;
    }
    return ink_led_attach(&led_ws2812_ops, NULL);
}

static const ink_device_ops_t s_dev_ops = { .init = led_init };
#endif

esp_err_t drv_led_register(const drv_led_cfg_t *cfg)
{
#if CONFIG_INK_LED_WS2812
    if (cfg->pin == GPIO_NUM_NC) {
        return ESP_OK; /* board không có LED */
    }
    s_cfg = *cfg;
    ink_device_t dev = { .name = "led", .comp = INK_COMP_LED, .ops = &s_dev_ops, .optional = true };
    return ink_device_register(&dev);
#else
    (void)cfg;
    return ESP_OK;
#endif
}
