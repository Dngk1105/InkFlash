/* Design: docs/design/code-structure.md */

/* Smoke test of the templates: the core handles events, the shell receives messages, the app table of the app template
 * is valid. Peripheral templates only need to build. */
#include "esp_err.h"
#include "esp_log.h"
#include "ink_example.h"
#include "ink_app_example.h"
#include "ink_example_shell.h"
#include "ink_keymap.h"

static const char *TAG = "templates";

void app_main(void) {
    ink_example_state_t state;
    if (ink_example_init(&state, 2) != INK_EXAMPLE_OK) {
        ESP_LOGE(TAG, "core init failed");
        return;
    }
    const ink_example_event_t tick   = {.type = INK_EXAMPLE_EVENT_TICK, .time_ms = 0};
    ink_example_action_t      action = INK_EXAMPLE_ACTION_NONE;
    for (int i = 0; i < 2; i++) {
        if (ink_example_handle(&state, &tick, &action) != INK_EXAMPLE_OK) {
            ESP_LOGE(TAG, "core handle failed");
            return;
        }
    }
    ESP_LOGI(TAG, "core action %d", action);

    if (ink_keymap_validate(nullptr, ink_app_example.app_table) != INK_KEYMAP_OK) {
        ESP_LOGE(TAG, "app table of %s is invalid", ink_app_example.name);
        return;
    }

    ESP_ERROR_CHECK(ink_example_shell_start());
    const ink_example_shell_message_t message = {.value = 1};
    ESP_ERROR_CHECK(ink_example_shell_post(&message));
}
