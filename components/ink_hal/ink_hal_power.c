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

bool ink_power_present(void) { return s_ops != NULL; }

esp_err_t ink_power_battery_mv(uint32_t *mv)
{
    if (!s_ops || !s_ops->battery_mv) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = s_ops->battery_mv(s_ctx, mv);
    ink_health_report(INK_COMP_CHARGER, err == ESP_OK);
    return err;
}

esp_err_t ink_power_battery_percent(uint8_t *percent)
{
    static const struct { uint16_t mv; uint8_t pct; } curve[] = {
        { 3300, 0 }, { 3500, 10 }, { 3700, 40 }, { 3800, 60 }, { 3900, 75 }, { 4000, 85 }, { 4200, 100 },
    };
    uint32_t mv;
    esp_err_t err = ink_power_battery_mv(&mv);
    if (err != ESP_OK) {
        return err;
    }
    const int n = sizeof(curve) / sizeof(curve[0]);
    if (mv <= curve[0].mv) {
        *percent = 0;
    } else if (mv >= curve[n - 1].mv) {
        *percent = 100;
    } else {
        int i = 1;
        while (mv > curve[i].mv) {
            i++;
        }
        *percent = curve[i - 1].pct + (curve[i].pct - curve[i - 1].pct) * (mv - curve[i - 1].mv) /
                                          (curve[i].mv - curve[i - 1].mv);
    }
    return ESP_OK;
}

esp_err_t ink_power_charge_state(ink_charge_state_t *out)
{
    if (!s_ops || !s_ops->charge_state) {
        return ESP_ERR_NOT_FOUND;
    }
    return s_ops->charge_state(s_ctx, out);
}

esp_err_t ink_power_set_rail(ink_rail_t rail, bool on)
{
    if (!s_ops || !s_ops->set_rail) {
        return ESP_ERR_NOT_FOUND;
    }
    if (rail >= INK_RAIL_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    return s_ops->set_rail(s_ctx, rail, on);
}

esp_err_t ink_power_deep_sleep(void)
{
    return (s_ops && s_ops->deep_sleep) ? s_ops->deep_sleep(s_ctx) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_power_ship_mode(void)
{
    return (s_ops && s_ops->ship_mode) ? s_ops->ship_mode(s_ctx) : ESP_ERR_NOT_FOUND;
}
