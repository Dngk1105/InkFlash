#include "ink_remote.h"
#include "ink_log.h"

esp_err_t ink_remote_init(void)
{
    /* TODO: WiFi chỉ bật khi đã thiết lập và cần sync; hàng đợi offline; OTA. */
    INK_LOGI(INK_COMP_REMOTE, INK_LOG_EVT_INIT, "stub");
    return ESP_OK;
}
