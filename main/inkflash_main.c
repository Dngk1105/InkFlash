#include <inttypes.h>
#include "esp_flash.h"
#include "esp_ota_ops.h"
#include "esp_psram.h"
#include "bsp.h"
#include "diag_console.h"
#include "ink_bus.h"
#include "ink_config.h"
#include "ink_device.h"
#include "ink_log.h"
#include "ink_remote.h"
#include "ink_service.h"

static void log_memory(void)
{
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    INK_LOGI(INK_COMP_SYS, INK_LOG_EVT_INIT, "flash %" PRIu32 " MB, psram %u MB",
             flash_size >> 20, (unsigned)(esp_psram_get_size() >> 20));
}

/* Firmware mới sau OTA: chỉ xác nhận khi self-test thiết bị bắt buộc qua hết. Nếu không, lần khởi động sau sẽ rollback. */
static void confirm_ota_if_healthy(esp_err_t selftest)
{
    esp_ota_img_states_t state;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (esp_ota_get_state_partition(running, &state) != ESP_OK || state != ESP_OTA_IMG_PENDING_VERIFY) {
        return;
    }
    if (selftest == ESP_OK) {
        esp_ota_mark_app_valid_cancel_rollback();
        INK_LOGI(INK_COMP_SYS, INK_LOG_EVT_STATE, "OTA image confirmed");
    } else {
        INK_LOGE(INK_COMP_SYS, INK_LOG_EVT_STATE, "self-test fail, rollback");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}

void app_main(void)
{
    /* Thứ tự khởi động: core → device registry + BSP → services → remote → console. */
    ESP_ERROR_CHECK(ink_config_init());
    ESP_ERROR_CHECK(ink_bus_init());
    log_memory();

    ESP_ERROR_CHECK(bsp_register_devices());
    esp_err_t selftest = ink_device_init_all();
    confirm_ota_if_healthy(selftest);
    /* TODO: safe mode khi crash liên tục (đếm số lần reset bất thường trong NVS). */

    ESP_ERROR_CHECK(ink_services_start());
    ESP_ERROR_CHECK(ink_remote_init());
    ESP_ERROR_CHECK(diag_console_start());
}
