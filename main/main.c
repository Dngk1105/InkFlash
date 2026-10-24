/* Thiết kế: docs/architecture.md */

/*
 * app_main chỉ lắp các khối với nhau, không có logic.
 * Hiện chưa có khối nào; chỉ in bản firmware lúc khởi động (docs/design/hardware-variants.md).
 */
#include "esp_app_desc.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    const esp_app_desc_t *app = esp_app_get_description();
    ESP_LOGI(TAG, "fw %s", app->version);
}
