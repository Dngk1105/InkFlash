#include "bsp.h"
#include "console.h"
#include "esp_ota_ops.h"
#include "ink_apps.h"
#include "ink_bus.h"
#include "ink_config.h"
#include "ink_device.h"
#include "ink_log.h"
#include "ink_service.h"

/*
 * Thứ tự khởi động: core → BSP (đăng ký driver) → init thiết bị → service → app manager → console.
 * GĐ3: đếm số lần crash liên tiếp (NVS + esp_reset_reason) và vào safe mode (mở app Settings) khi quá ngưỡng.
 */
static void confirm_ota_image(esp_err_t devices)
{
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) != ESP_OK ||
        state != ESP_OTA_IMG_PENDING_VERIFY) {
        return;
    }
    if (devices == ESP_OK) {
        esp_ota_mark_app_valid_cancel_rollback();
        INK_LOGI(INK_COMP_SYS, "firmware OTA da xac nhan");
    } else {
        INK_LOGE(INK_COMP_SYS, "thiet bi bat buoc hong: quay lai firmware cu");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(ink_config_init());
    ESP_ERROR_CHECK(ink_bus_init());

    ESP_ERROR_CHECK(bsp_init());
    const esp_err_t devices = ink_device_init_all();
    confirm_ota_image(devices);

    ESP_ERROR_CHECK(ink_services_start());
    ESP_ERROR_CHECK(app_manager_start(&app_flashcard));
    ESP_ERROR_CHECK(console_start());
}
