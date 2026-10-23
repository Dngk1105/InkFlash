#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL nguồn: pin, sạc, nguồn khối phụ, ngủ sâu, ship mode.
 * Service power quyết định KHI NÀO ngủ; driver quyết định NGỦ THẾ NÀO (nguồn đánh thức, cắt nguồn khối phụ).
 */
typedef enum {
    INK_RAIL_DISPLAY,
    INK_RAIL_AUDIO,
    INK_RAIL_SENSORS,
    INK_RAIL_COUNT,
} ink_rail_t;

typedef enum {
    INK_CHARGE_UNKNOWN,
    INK_CHARGE_NONE, /* không cắm sạc */
    INK_CHARGE_CHARGING,
    INK_CHARGE_DONE,
    INK_CHARGE_FAULT,
} ink_charge_t;

typedef enum {
    INK_WAKE_COLD_BOOT,  /* cấp nguồn hoặc reset */
    INK_WAKE_BUTTON,
    INK_WAKE_TIMER,
    INK_WAKE_CHARGER,
    INK_WAKE_OTHER,
} ink_wake_t;

typedef struct {
    esp_err_t (*battery_mv)(void *ctx, uint32_t *mv);
    esp_err_t (*charge_state)(void *ctx, ink_charge_t *out);
    esp_err_t (*set_rail)(void *ctx, ink_rail_t rail, bool on);
    /* Cấu hình nguồn đánh thức (nút; hẹn giờ nếu timer_s > 0) rồi ngủ. Thật thì không quay về. */
    esp_err_t (*deep_sleep)(void *ctx, uint32_t timer_s);
    esp_err_t (*ship_mode)(void *ctx); /* ngắt pin; thật thì không quay về */
    ink_wake_t (*wake_cause)(void *ctx);
} ink_power_ops_t;

/* Phía driver */
esp_err_t ink_power_attach(const ink_power_ops_t *ops, void *ctx);

/* Phía service. Hàm nào driver không có thì trả ESP_ERR_NOT_SUPPORTED. */
bool ink_power_present(void);
esp_err_t ink_power_battery_mv(uint32_t *mv);
esp_err_t ink_power_charge_state(ink_charge_t *out);
esp_err_t ink_power_set_rail(ink_rail_t rail, bool on);
esp_err_t ink_power_deep_sleep(uint32_t timer_s);
esp_err_t ink_power_ship_mode(void);
ink_wake_t ink_power_wake_cause(void);
