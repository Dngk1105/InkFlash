#include "ink_hal_power.h"
#include "ink_health.h"

static const ink_power_ops_t *s_ops;
static void *s_ctx;

esp_err_t ink_power_attach(const ink_power_ops_t *ops, void *ctx)
{
    if (!ops) {
        return ESP_ERR_INVALID_ARG;
    }
    s_ctx = ctx;
    s_ops = ops;
    return ESP_OK;
}

bool ink_power_present(void)
{
    return s_ops != NULL;
}

#define CALL(fn, ...)                                                   \
    (!s_ops ? ESP_ERR_NOT_FOUND : !s_ops->fn ? ESP_ERR_NOT_SUPPORTED : s_ops->fn(s_ctx, ##__VA_ARGS__))

esp_err_t ink_power_battery_mv(uint32_t *mv)
{
    esp_err_t err = CALL(battery_mv, mv);
    if (err != ESP_ERR_NOT_FOUND && err != ESP_ERR_NOT_SUPPORTED) {
        ink_health_report(INK_COMP_POWER_HW, err == ESP_OK);
    }
    return err;
}

esp_err_t ink_power_charge_state(ink_charge_t *out)
{
    return CALL(charge_state, out);
}

esp_err_t ink_power_set_rail(ink_rail_t rail, bool on)
{
    if (rail >= INK_RAIL_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    return CALL(set_rail, rail, on);
}

esp_err_t ink_power_deep_sleep(uint32_t timer_s)
{
    return CALL(deep_sleep, timer_s);
}

esp_err_t ink_power_ship_mode(void)
{
    return CALL(ship_mode);
}

ink_wake_t ink_power_wake_cause(void)
{
    return (s_ops && s_ops->wake_cause) ? s_ops->wake_cause(s_ctx) : INK_WAKE_COLD_BOOT;
}
