/* Thiết kế: docs/design/code-structure.md */

/* Chạy thử các template: lõi xử lý sự kiện, vỏ nhận message. Mẫu ngoại vi chỉ cần build được. */
#include "esp_err.h"
#include "esp_log.h"
#include "ink_example.h"
#include "ink_example_shell.h"

static const char *TAG = "templates";

void app_main(void)
{
    ink_example_state_t state;
    if (ink_example_init(&state, 2) != INK_EXAMPLE_OK) {
        ESP_LOGE(TAG, "core init failed");
        return;
    }
    const ink_example_event_t tick = {.type = INK_EXAMPLE_EVENT_TICK, .time_ms = 0};
    ink_example_action_t action = INK_EXAMPLE_ACTION_NONE;
    for (int i = 0; i < 2; i++) {
        if (ink_example_handle(&state, &tick, &action) != INK_EXAMPLE_OK) {
            ESP_LOGE(TAG, "core handle failed");
            return;
        }
    }
    ESP_LOGI(TAG, "core action %d", action);

    ESP_ERROR_CHECK(ink_example_shell_start());
    const ink_example_shell_msg_t msg = {.value = 1};
    ESP_ERROR_CHECK(ink_example_shell_post(&msg));
}
