/* Design: docs/architecture.md */

/* app_main only wires modules together; no logic. Logs the firmware version at boot (hardware-variants.md). */
#include <stddef.h>
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ink_bsp.h"
#include "ink_system.h"

static const char *TAG = "main";

void app_main(void) {
    const esp_app_desc_t *app_description = esp_app_get_description();
    /* TODO(main): log the board and detected chips too, as hardware-variants.md requires. */
    ESP_LOGI(TAG, "fw %s", app_description->version);

    ESP_ERROR_CHECK(ink_bsp_init());
    const ink_hal_button_device_t *buttons      = nullptr;
    size_t                         button_count = 0;
    ESP_ERROR_CHECK(ink_bsp_buttons(&buttons, &button_count));
    const ink_hal_display_device_t *display = nullptr;
    ESP_ERROR_CHECK(ink_bsp_display(&display));
    const ink_system_config_t config = {
        .buttons      = buttons,
        .button_count = button_count,
        .display      = display,
    };
    ESP_ERROR_CHECK(ink_system_start(&config));
}
