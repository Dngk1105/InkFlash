/* Thiết kế: docs/design/input.md */
#pragma once

/* Gesture → intent theo bảng gán của scene và app. Hàm không giữ trạng thái:
 * system giữ các bảng. */
#include "ink_gesture.h"

#include <stdint.h>

/* ---- Intent ---- */
/* Mã lưu dạng số trong bảng gán: intent chung chỉ thêm vào chỗ trống 4..15,
 * intent riêng của app đánh mã từ INK_KEYMAP_INTENT_APP_BASE, ví dụ
 * (ink_keymap_intent_t)(INK_KEYMAP_INTENT_APP_BASE + 1). */

typedef enum : uint8_t {
  INK_KEYMAP_INTENT_NEXT = 0,
  INK_KEYMAP_INTENT_PREV = 1,
  INK_KEYMAP_INTENT_SELECT = 2,
  INK_KEYMAP_INTENT_BACK = 3,
  INK_KEYMAP_INTENT_APP_BASE = 16,
} ink_keymap_intent_t;

/* ---- Bảng gán ---- */

#define INK_KEYMAP_ENTRY_MAX 32

typedef struct {
  ink_gesture_t gesture;
  ink_keymap_intent_t intent;
} ink_keymap_entry_t;

/* Bảng của app hoặc của một scene (chỉ gồm các dòng khác bảng app). Bên gọi giữ
 * và sửa trực tiếp. */
typedef struct {
  uint8_t count;
  ink_keymap_entry_t entries[INK_KEYMAP_ENTRY_MAX];
} ink_keymap_table_t;

/* ---- Lỗi ---- */

typedef enum : uint8_t {
  INK_KEYMAP_OK = 0,
  INK_KEYMAP_ERROR_ARGUMENT,
  INK_KEYMAP_ERROR_COUNT,     /* count vượt INK_KEYMAP_ENTRY_MAX */
  INK_KEYMAP_ERROR_RESERVED,  /* gán gesture của hệ thống (giữ A+B ngắn, dài) */
  INK_KEYMAP_ERROR_DUPLICATE, /* một gesture hai dòng trong cùng bảng */
  INK_KEYMAP_ERROR_NO_BACK,   /* không gesture nào ra BACK */
} ink_keymap_error_t;

/* ---- API ---- */

/*
 * Tra intent của gesture: bảng scene trước, không có thì bảng app. Trả false
 * nếu cả hai không gán (gesture bị bỏ). scene có thể là nullptr. system gọi cho
 * mọi gesture trừ giữ A+B ngắn và dài.
 */
bool ink_keymap_lookup(const ink_keymap_table_t *scene,
                       const ink_keymap_table_t *app,
                       const ink_gesture_t *gesture,
                       ink_keymap_intent_t *out_intent);

/*
 * Kiểm tra bảng hiệu dụng (scene gộp với app). system gọi trước khi nhận bảng
 * người dùng sửa, và khi app khai bảng scene. Trả ERROR_ARGUMENT nếu app là
 * nullptr. scene có thể là nullptr.
 */
[[nodiscard]] ink_keymap_error_t
ink_keymap_validate(const ink_keymap_table_t *scene,
                    const ink_keymap_table_t *app);

/* Chép bảng mặc định vào bảng đang dùng. Trả ERROR_ARGUMENT nếu có con trỏ
 * nullptr. */
[[nodiscard]] ink_keymap_error_t
ink_keymap_reset(ink_keymap_table_t *table, const ink_keymap_table_t *defaults);

/*
 * Mask kiểu phải chờ (bit INK_GESTURE_WAIT_*) của một nút theo bảng đang dùng,
 * để system đưa cho ink_gesture_set_enabled khi mở app hoặc đổi scene. button
 * không phải A, B thì trả INK_GESTURE_WAIT_NONE.
 */
uint8_t ink_keymap_wait_mask(const ink_keymap_table_t *scene,
                             const ink_keymap_table_t *app,
                             ink_gesture_button_t button);
