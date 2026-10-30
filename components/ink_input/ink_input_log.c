/* Thiết kế: docs/design/input.md */

/* Đổi mã gesture thành chữ và in log. Hàm thuần, không giữ trạng thái; gọi từ task main. */
#include "esp_log.h"
#include "ink_input_internal.h"

static const char *TAG = "ink_input";

/* ---- Tên ---- */

const char *input_button_name(uint8_t button) {
    switch (button) {
        case INK_GESTURE_BUTTON_A:
            return "A";
        case INK_GESTURE_BUTTON_B:
            return "B";
        case INK_GESTURE_BUTTON_AB:
            return "AB";
        default:
            return "-";
    }
}

static const char *gesture_type_name(ink_gesture_type_t type) {
    switch (type) {
        case INK_GESTURE_TYPE_PRESS:
            return "press";
        case INK_GESTURE_TYPE_DOUBLE_PRESS:
            return "double";
        case INK_GESTURE_TYPE_HOLD:
            return "hold";
        case INK_GESTURE_TYPE_HOLD_AB_SHORT:
            return "hold_ab_short";
        case INK_GESTURE_TYPE_HOLD_AB_LONG:
            return "hold_ab_long";
        default:
            return "?";
    }
}

/* ---- Log gesture và progress ---- */

void input_log_gestures(const ink_gesture_output_t *gestures) {
    for (uint8_t i = 0; i < gestures->count; i++) {
        const ink_gesture_t *gesture = &gestures->items[i];
        ESP_LOGI(TAG, "gesture modifier=%s button=%s type=%s", input_button_name(gesture->modifier), input_button_name(gesture->button),
                 gesture_type_name(gesture->type));
    }
}

void input_log_progress(const ink_gesture_progress_t *progress, uint32_t now_ms) {
    const unsigned long held_ms = now_ms - progress->start_ms;
    if (progress->has_next) {
        ESP_LOGI(TAG, "progress modifier=%s button=%s held=%lu ms release=%s next=%s in %ld ms", input_button_name(progress->modifier),
                 input_button_name(progress->button), held_ms, gesture_type_name(progress->on_release), gesture_type_name(progress->next),
                 (long)(int32_t)(progress->next_ms - now_ms));
    } else {
        ESP_LOGI(TAG, "progress modifier=%s button=%s held=%lu ms release=%s", input_button_name(progress->modifier),
                 input_button_name(progress->button), held_ms, gesture_type_name(progress->on_release));
    }
}
