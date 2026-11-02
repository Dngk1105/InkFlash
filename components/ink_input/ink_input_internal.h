/* Thiết kế: docs/design/input/input.md */
#pragma once

/* Hàm dùng chung giữa các file của ink_input. Không nằm trong include/: khối khác không thấy. */
#include "ink_input.h"
#include <stddef.h>
#include <stdint.h>

/* ---- ink_input_log.c ---- */

const char *input_button_name(uint8_t button);

void input_log_gestures(const ink_gesture_output_t *gestures);

void input_log_progress(const ink_gesture_progress_t *progress, uint32_t now_ms);
