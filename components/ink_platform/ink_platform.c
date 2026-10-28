/* Thiết kế: docs/design/input.md */
#include "ink_platform.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_console.h"
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

/* ---- Console ---- */

static esp_console_repl_t *s_console_repl;

esp_err_t ink_platform_console_init(const char *prompt) {
    ESP_RETURN_ON_FALSE(s_console_repl == nullptr, ESP_ERR_INVALID_STATE, TAG, "console already init");
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt                    = prompt;
    return esp_console_new_repl_stdio(&repl_config, &s_console_repl);
}

esp_err_t ink_platform_console_add(const char *name, const char *help, const char *hint, ink_platform_command_handler_t handler) {
    ESP_RETURN_ON_FALSE(s_console_repl, ESP_ERR_INVALID_STATE, TAG, "console not init");
    const esp_console_cmd_t command = {
        .command = name,
        .help    = help,
        .hint    = hint,
        .func    = handler,
    };
    return esp_console_cmd_register(&command);
}

esp_err_t ink_platform_console_start(void) {
    ESP_RETURN_ON_FALSE(s_console_repl, ESP_ERR_INVALID_STATE, TAG, "console not init");
    return esp_console_start_repl(s_console_repl);
}
