/* Thiết kế: docs/design/display/display.md */
#pragma once

/* Hàm dùng chung giữa các file của ink_display. Không nằm trong include/: khối khác không thấy. */
#include "ink_display.h"
#include <stddef.h>
#include <stdint.h>

/* ---- ink_display.c ---- */
/* Gọi từ task khác task màn: chỉ ghi yêu cầu trong lock rồi notify, task màn làm và log kết quả. */

bool display_is_started(void);

/* Bảng khả năng của driver; không đổi sau ink_display_start(). */
const ink_hal_display_capabilities_t *display_capabilities(void);

/* Profile đang gán cho slot ở dải nhiệt độ đầu. */
size_t display_slot_profile(ink_hal_display_slot_t slot);

/* `profile` là index trong capabilities.profiles, bên gọi đã kiểm. */
void display_request_profile(ink_hal_display_slot_t slot, size_t profile);

/* 0 = tắt đẩy lại viền; quá INK_DISPLAY_POLICY_MAX_EDGE_MARGIN thì task màn từ chối và log lỗi. */
void display_request_edge_margin(uint8_t margin);

void display_request_temperature(void);

void display_request_stats(void);
