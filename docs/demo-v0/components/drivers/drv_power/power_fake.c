#include "drv_power.h"
#include "ink_log.h"

static bool s_rail[INK_RAIL_COUNT];

static esp_err_t fake_battery_mv(void *ctx, uint32_t *mv)
{
    *mv = 3900;
    return ESP_OK;
}

static esp_err_t fake_charge_state(void *ctx, ink_charge_state_t *out)
{
    *out = INK_CHARGE_NONE;
    return ESP_OK;
}

static esp_err_t fake_set_rail(void *ctx, ink_rail_t rail, bool on)
{
    s_rail[rail] = on;
    INK_LOGI(INK_COMP_CHARGER, INK_LOG_EVT_STATE, "fake rail %d -> %s", (int)rail, on ? "on" : "off");
    return ESP_OK;
}

/* Giả lập không ngủ thật, nên hàm quay về. Driver thật sẽ không quay về. */
static esp_err_t fake_deep_sleep(void *ctx)
{
    const drv_power_cfg_t *cfg = ctx;
    INK_LOGI(INK_COMP_CHARGER, INK_LOG_EVT_STATE, "fake deep sleep (wake_mask=0x%llx)",
             (unsigned long long)cfg->wake_mask);
    return ESP_OK;
}

static esp_err_t fake_ship_mode(void *ctx)
{
    INK_LOGI(INK_COMP_CHARGER, INK_LOG_EVT_STATE, "fake ship mode");
    return ESP_OK;
}

const ink_power_ops_t power_fake_ops = {
    .battery_mv = fake_battery_mv,
    .charge_state = fake_charge_state,
    .set_rail = fake_set_rail,
    .deep_sleep = fake_deep_sleep,
    .ship_mode = fake_ship_mode,
};
