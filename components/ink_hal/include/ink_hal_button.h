#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL nút bấm. Driver chỉ báo thay đổi mức THÔ (đã chống dội ở mức điện nếu muốn), không nhận diện cử chỉ.
 * Nhận diện nhấn đơn/kép/giữ/tổ hợp nằm ở service input (logic, test được trên PC).
 *
 * Luồng: ngắt GPIO → driver gọi ink_button_report_from_isr() → HAL phát INK_EVT_BUTTON_RAW lên bus.
 * Chỉ số nút (0 = A, 1 = B, ...) do BSP quy định, không phải số GPIO.
 */
typedef struct {
    uint8_t (*count)(void *ctx);
    bool (*is_pressed)(void *ctx, uint8_t index); /* đọc trạng thái hiện tại, dùng khi thức dậy */
} ink_button_ops_t;

/* Phía driver */
esp_err_t ink_button_attach(const ink_button_ops_t *ops, void *ctx);
void ink_button_report(uint8_t index, bool pressed);         /* gọi từ task */
void ink_button_report_from_isr(uint8_t index, bool pressed); /* gọi từ ISR */

/* Phía service */
bool ink_button_present(void);
uint8_t ink_button_count(void);
bool ink_button_is_pressed(uint8_t index);
