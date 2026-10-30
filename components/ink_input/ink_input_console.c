/* Thiết kế: docs/design/input/input.md */

/* Lệnh console `gesture`. Chạy trên task REPL: chỉ kiểm tra cú pháp rồi đẩy message; task main làm và log kết quả. */
#include "ink_input_internal.h"
#include "ink_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Bảng trường config gesture ---- */

typedef struct {
    const char *name;
    size_t      offset;
} config_field_t;

static const config_field_t s_config_fields[] = {
    {"debounce_ms", offsetof(ink_gesture_config_t, debounce_ms)},
    {"double_ms",   offsetof(ink_gesture_config_t, double_ms)  },
    {"hold_ms",     offsetof(ink_gesture_config_t, hold_ms)    },
    {"combo_ms",    offsetof(ink_gesture_config_t, combo_ms)   },
    {"ab_short_ms", offsetof(ink_gesture_config_t, ab_short_ms)},
    {"ab_long_ms",  offsetof(ink_gesture_config_t, ab_long_ms) },
};

#define CONFIG_FIELD_COUNT (sizeof(s_config_fields) / sizeof(s_config_fields[0]))

size_t input_config_field_count(void) {
    return CONFIG_FIELD_COUNT;
}

const char *input_config_field_name(size_t field_index) {
    return s_config_fields[field_index].name;
}

uint32_t *input_config_field_value(ink_gesture_config_t *config, size_t field_index) {
    return (uint32_t *)((uint8_t *)config + s_config_fields[field_index].offset);
}

/* ---- Phân tích lệnh ---- */

static bool parse_uint32(const char *text, uint32_t *out_value) {
    char         *end   = nullptr;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value > UINT32_MAX) { return false; }
    *out_value = (uint32_t)value;
    return true;
}

static bool parse_set_config(char **argv, ink_input_message_t *out_message) {
    size_t field_index = 0;
    while (field_index < CONFIG_FIELD_COUNT && strcmp(argv[2], s_config_fields[field_index].name) != 0) { field_index++; }
    if (field_index == CONFIG_FIELD_COUNT || !parse_uint32(argv[3], &out_message->set_config.value_ms)) {
        printf("gesture set <debounce_ms|double_ms|hold_ms|combo_ms|ab_short_ms|ab_long_ms> <ms>\n");
        return false;
    }
    out_message->type                   = INK_INPUT_MESSAGE_SET_CONFIG;
    out_message->set_config.field_index = (uint8_t)field_index;
    return true;
}

static bool parse_set_wait_mask(char **argv, ink_input_message_t *out_message) {
    const bool is_button_a = strcmp(argv[2], "a") == 0;
    const bool is_button_b = strcmp(argv[2], "b") == 0;
    uint32_t   wait_mask   = 0;
    if (!(is_button_a || is_button_b) || !parse_uint32(argv[3], &wait_mask) || wait_mask > UINT8_MAX) {
        printf("gesture enable <a|b> <mask>  (1 = double, 2 = modifier)\n");
        return false;
    }
    out_message->type                    = INK_INPUT_MESSAGE_SET_WAIT_MASK;
    out_message->set_wait_mask.button    = is_button_a ? INK_GESTURE_BUTTON_A : INK_GESTURE_BUTTON_B;
    out_message->set_wait_mask.wait_mask = (uint8_t)wait_mask;
    return true;
}

static int handle_gesture_command(int argc, char **argv) {
    ink_input_message_t message = {0};
    bool                valid   = false;
    if (argc == 2 && strcmp(argv[1], "show") == 0) {
        message.type = INK_INPUT_MESSAGE_SHOW_CONFIG;
        valid        = true;
    } else if (argc == 4 && strcmp(argv[1], "set") == 0) {
        valid = parse_set_config(argv, &message);
    } else if (argc == 4 && strcmp(argv[1], "enable") == 0) {
        valid = parse_set_wait_mask(argv, &message);
    } else {
        printf("gesture show | set <field> <ms> | enable <a|b> <mask>\n");
    }
    if (!valid) { return 1; }
    input_post(&message);
    return 0;
}

/* ---- Đăng ký lệnh ---- */

esp_err_t ink_input_add_console_command(void) {
    if (!input_is_started()) { return ESP_ERR_INVALID_STATE; }
    return ink_platform_console_add("gesture", "Xem, đổi ngưỡng gesture và mask kiểu phải chờ",
                                    "show | set <field> <ms> | enable <a|b> <mask>", handle_gesture_command);
}
