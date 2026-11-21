/* Design: docs/architecture.md */

/* app_main only wires modules together; no logic. Logs firmware version and board at boot (hardware-variants.md).
 * Any boot error is fatal: ESP_ERROR_CHECK aborts, and no module rolls back a failed start. */
#include <stddef.h>
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ink_app_home.h"
#include "ink_bsp.h"
#include "ink_system.h"

static const char *TAG = "main";

/* Apps built into the firmware; apps[0] is Home. EXTEND(main): add each new app here (tools/new.py app prints the reminder). */
static const ink_system_app_t *const s_apps[] = {&ink_app_home};

#define IDLE_MS 60000 /* no gesture for 60 s -> deep sleep (docs/design/wake-session.md) */

void app_main(void) {
    const esp_app_desc_t *app_description = esp_app_get_description();
    ESP_ERROR_CHECK(ink_bsp_init());
    /* TODO(main): add detected chips (IMU...) to this line once bsp probes them, as hardware-variants.md requires. */
    ESP_LOGI(TAG, "fw %s board %s", app_description->version, ink_bsp_board_name());
    const ink_hal_button_device_t *buttons      = nullptr;
    size_t                         button_count = 0;
    ESP_ERROR_CHECK(ink_bsp_buttons(&buttons, &button_count));
    const ink_hal_display_device_t *display = nullptr;
    ESP_ERROR_CHECK(ink_bsp_display(&display));
    const ink_system_config_t config = {
        .buttons       = buttons,
        .button_count  = button_count,
        .display       = display,
        .apps          = s_apps,
        .app_count     = sizeof(s_apps) / sizeof(s_apps[0]),
        .idle_ms       = IDLE_MS,
        .wake_pin_mask = ink_bsp_wake_pin_mask(),
    };
    ESP_ERROR_CHECK(ink_system_start(&config));
}
