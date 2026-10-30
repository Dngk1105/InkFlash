/* Thiết kế: docs/design/input.md */
#pragma once

/* Hàm dùng chung giữa các file của ink_input. Không nằm trong include/: khối khác không thấy. */
#include "ink_input.h"
#include <stddef.h>
#include <stdint.h>

/* ---- ink_input.c ---- */

/* Đẩy message vào hàng đợi task main qua hàm post của system. Gọi từ task (REPL, timer), không gọi từ ISR. */
void input_post(const ink_input_message_t *message);

bool input_is_started(void);

/* ---- ink_input_console.c ---- */

/* Bảng trường config gesture: dùng chung cho lệnh console (tìm theo tên) và task main (đọc, ghi theo index). */
size_t input_config_field_count(void);

const char *input_config_field_name(size_t field_index);

uint32_t *input_config_field_value(ink_gesture_config_t *config, size_t field_index);

/* ---- ink_input_log.c ---- */

const char *input_button_name(uint8_t button);

void input_log_gestures(const ink_gesture_output_t *gestures);

void input_log_progress(const ink_gesture_progress_t *progress, uint32_t now_ms);
