#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL nguồn: pin, sạc, nguồn các khối phụ, ngủ sâu, ship mode.
 * Logic (service Power) không biết chip sạc là BQ25180 hay chip khác, và các khối phụ cấp nguồn bằng TPS22916 hay cách nào.
 */
typedef enum {
    INK_RAIL_DISPLAY,
    INK_RAIL_AUDIO,
    INK_RAIL_SENSORS,
    INK_RAIL_COUNT,
} ink_rail_t;

typedef enum {
    INK_CHARGE_NONE, /* không cắm sạc */
    INK_CHARGE_CHARGING,
    INK_CHARGE_DONE,
    INK_CHARGE_FAULT,
} ink_charge_state_t;

typedef struct {
    esp_err_t (*battery_mv)(void *ctx, uint32_t *mv);
    esp_err_t (*charge_state)(void *ctx, ink_charge_state_t *out);
    esp_err_t (*set_rail)(void *ctx, ink_rail_t rail, bool on);
    esp_err_t (*deep_sleep)(void *ctx); /* cấu hình nguồn đánh thức rồi ngủ; thật thì không quay về */
    esp_err_t (*ship_mode)(void *ctx);  /* ngắt pin; thật thì không quay về */
} ink_power_ops_t;

esp_err_t ink_power_attach(const ink_power_ops_t *ops, void *ctx);

bool ink_power_present(void);
esp_err_t ink_power_battery_mv(uint32_t *mv);
/* Xấp xỉ theo đường cong LiPo; chỉnh bảng trong ink_hal_power.c sau khi đo pin thật. */
esp_err_t ink_power_battery_percent(uint8_t *percent);
esp_err_t ink_power_charge_state(ink_charge_state_t *out);
esp_err_t ink_power_set_rail(ink_rail_t rail, bool on);
esp_err_t ink_power_deep_sleep(void);
esp_err_t ink_power_ship_mode(void);
