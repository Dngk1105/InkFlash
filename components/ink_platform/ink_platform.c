/* Thiết kế: docs/design/input/input.md */
#include "ink_platform.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_timer.h"

static const char *TAG = "ink_platform";

/* ---- Giờ ---- */

uint32_t IRAM_ATTR ink_platform_now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/* ---- Timer một lần ---- */
/* ink_platform_timer_t chính là esp_timer_handle_t, ép kiểu qua lại. */

esp_err_t ink_platform_timer_create(ink_platform_timer_callback_t callback, void *user_data, const char *name,
                                    ink_platform_timer_t *out_timer) {
    ESP_RETURN_ON_FALSE(callback && out_timer, ESP_ERR_INVALID_ARG, TAG, "null arg");
    const esp_timer_create_args_t timer_args = {
        .callback        = callback,
        .arg             = user_data,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = name,
    };
    esp_timer_handle_t handle = nullptr;
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &handle), TAG, "esp_timer_create");
    *out_timer = (ink_platform_timer_t)handle;
    return ESP_OK;
}

esp_err_t ink_platform_timer_start(ink_platform_timer_t timer, uint32_t delay_ms) {
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "null timer");
    ESP_RETURN_ON_ERROR(ink_platform_timer_stop(timer), TAG, "stop");
    return esp_timer_start_once((esp_timer_handle_t)timer, (uint64_t)delay_ms * 1000);
}

esp_err_t ink_platform_timer_stop(ink_platform_timer_t timer) {
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "null timer");
    const esp_err_t error = esp_timer_stop((esp_timer_handle_t)timer);
    /* ESP_ERR_INVALID_STATE: timer không chạy */
    return error == ESP_ERR_INVALID_STATE ? ESP_OK : error;
}
