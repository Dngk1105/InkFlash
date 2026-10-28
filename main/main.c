/* Thiết kế: docs/architecture.md */

/* app_main chỉ lắp các khối với nhau, không có logic. In bản firmware lúc khởi động
 * (docs/design/hardware-variants.md). */
#include <stddef.h>
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ink_bsp.h"
#include "ink_system.h"

static const char *TAG = "main";

void app_main(void) {
    const esp_app_desc_t *app_description = esp_app_get_description();
    ESP_LOGI(TAG, "fw %s", app_description->version);

    ESP_ERROR_CHECK(ink_bsp_init());
    const ink_hal_button_device_t *buttons      = nullptr;
    size_t                         button_count = 0;
    ESP_ERROR_CHECK(ink_bsp_buttons(&buttons, &button_count));
    ESP_ERROR_CHECK(ink_system_start(buttons, button_count));
}
