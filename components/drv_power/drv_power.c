#include "drv_power.h"
#include "ink_device.h"
#include "sdkconfig.h"

#if CONFIG_INK_POWER_FAKE
extern const ink_power_ops_t power_fake_ops;
#define POWER_OPS (&power_fake_ops)
#endif

#ifdef POWER_OPS
static drv_power_cfg_t s_cfg;
static esp_err_t power_init(void *ctx) { return ink_power_attach(POWER_OPS, &s_cfg); }
static const ink_device_ops_t s_dev_ops = { .init = power_init };
#endif

esp_err_t drv_power_register(const drv_power_cfg_t *cfg)
{
#ifdef POWER_OPS
    s_cfg = *cfg;
    ink_device_t dev = { .name = "power", .comp = INK_COMP_CHARGER, .ops = &s_dev_ops, .optional = true };
    return ink_device_register(&dev);
#else
    (void)cfg;
    return ESP_OK;
#endif
}
