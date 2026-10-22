#include "drv_display.h"
#include "drv_display_chip.h"
#include "ink_device.h"
#include "sdkconfig.h"

#if CONFIG_INK_DISPLAY_FAKE
extern const drv_display_chip_t display_fake_chip;
#define DISPLAY_CHIP (&display_fake_chip)
#elif CONFIG_INK_DISPLAY_UC8253
extern const drv_display_chip_t display_uc8253_chip;
#define DISPLAY_CHIP (&display_uc8253_chip)
#endif

#ifdef DISPLAY_CHIP
static drv_display_cfg_t s_cfg;

static esp_err_t display_init(void *ctx)
{
    if (DISPLAY_CHIP->init) {
        esp_err_t err = DISPLAY_CHIP->init(&s_cfg);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ink_display_attach(DISPLAY_CHIP->ops, &s_cfg);
}

static esp_err_t display_suspend(void *ctx)
{
    return ink_display_sleep();
}

/* Mức 1: reset lại panel. Mức 2 (tắt/bật nguồn) cần công tắc tải, breadboard chưa có. */
static esp_err_t display_recover(void *ctx, int level)
{
    return level == 1 ? display_init(ctx) : ESP_ERR_NOT_SUPPORTED;
}

static const ink_device_ops_t s_dev_ops = {
    .init = display_init,
    .suspend = display_suspend,
    .recover = display_recover,
};
#endif

esp_err_t drv_display_register(const drv_display_cfg_t *cfg)
{
#ifdef DISPLAY_CHIP
    s_cfg = *cfg;
    ink_device_t dev = {
        .name = "display",
        .comp = INK_COMP_EPD,
        .ops = &s_dev_ops,
        .optional = true,
    };
    return ink_device_register(&dev);
#else
    (void)cfg;
    return ESP_OK;
#endif
}
