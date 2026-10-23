#include "drv_power.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "ink_log.h"

static bool s_rail[INK_RAIL_COUNT];

static esp_err_t esp32_battery_mv(void *ctx, uint32_t *mv)
{
    (void)ctx;
    /* Giá trị danh định khi chưa đấu ADC pin hoặc chip sạc BQ25180 */
    *mv = 3900;
    return ESP_OK;
}

static esp_err_t esp32_charge_state(void *ctx, ink_charge_state_t *out)
{
    (void)ctx;
    *out = INK_CHARGE_NONE;
    return ESP_OK;
}

static esp_err_t esp32_set_rail(void *ctx, ink_rail_t rail, bool on)
{
    const drv_power_cfg_t *cfg = ctx;
    if (rail < 0 || rail >= INK_RAIL_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    s_rail[rail] = on;
    if (cfg && cfg->rail_en[rail] != GPIO_NUM_NC) {
        gpio_set_level(cfg->rail_en[rail], on ? 1 : 0);
    }
    INK_LOGI(INK_COMP_CHARGER, INK_LOG_EVT_STATE, "rail %d -> %s", (int)rail, on ? "on" : "off");
    return ESP_OK;
}

static esp_err_t esp32_deep_sleep(void *ctx)
{
    const drv_power_cfg_t *cfg = ctx;
    INK_LOGI(INK_COMP_CHARGER, INK_LOG_EVT_STATE, "entering deep sleep (wake_mask=0x%llx)",
             cfg ? (unsigned long long)cfg->wake_mask : 0ULL);

    if (cfg && cfg->wake_mask != 0) {
        for (int pin = 0; pin < 32; pin++) {
            if (cfg->wake_mask & (1ULL << pin)) {
                gpio_pullup_en((gpio_num_t)pin);
                gpio_pulldown_dis((gpio_num_t)pin);
            }
        }
        esp_sleep_enable_ext1_wakeup_io(cfg->wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    }

    /* Tắt tất cả các rail phụ trước khi vào deep sleep */
    if (cfg) {
        for (int r = 0; r < INK_RAIL_COUNT; r++) {
            if (cfg->rail_en[r] != GPIO_NUM_NC) {
                gpio_set_level(cfg->rail_en[r], 0);
            }
        }
    }

    esp_deep_sleep_start();
    return ESP_OK; /* Driver thật sẽ không quay về */
}

static esp_err_t esp32_ship_mode(void *ctx)
{
    INK_LOGW(INK_COMP_CHARGER, INK_LOG_EVT_STATE, "ship mode requested (no BQ25180, fallback to deep sleep)");
    return esp32_deep_sleep(ctx);
}

const ink_power_ops_t power_esp32_ops = {
    .battery_mv = esp32_battery_mv,
    .charge_state = esp32_charge_state,
    .set_rail = esp32_set_rail,
    .deep_sleep = esp32_deep_sleep,
    .ship_mode = esp32_ship_mode,
};
