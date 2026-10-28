/* Thiết kế: docs/design/input.md */
#pragma once

/* Edge nút → gesture. Không đọc đồng hồ: bên gọi tick theo
 * ink_gesture_next_deadline(). */
#include <stdint.h>

/* ---- Mã gesture ---- */
/* Enum chỉ thêm giá trị vào cuối: keymap đã lưu dạng số, đổi thứ tự là trỏ sai
 * gesture. Mã nút và modifier là bitmask: mỗi nút một bit, tổ hợp là OR các
 * bit, thêm nút là thêm bit. */

typedef enum : uint8_t {
    INK_GESTURE_SOURCE_BUTTON = 0,
} ink_gesture_source_t;

/* Edge đầu vào chỉ một nút; tổ hợp chỉ có ở gesture đầu ra. */
typedef enum : uint8_t {
    INK_GESTURE_BUTTON_A  = 1 << 0,
    INK_GESTURE_BUTTON_B  = 1 << 1,
    INK_GESTURE_BUTTON_AB = INK_GESTURE_BUTTON_A | INK_GESTURE_BUTTON_B,
} ink_gesture_button_t;

typedef enum : uint8_t {
    INK_GESTURE_TYPE_PRESS = 0,
    INK_GESTURE_TYPE_DOUBLE_PRESS,
    INK_GESTURE_TYPE_HOLD,
    INK_GESTURE_TYPE_HOLD_AB_SHORT,
    INK_GESTURE_TYPE_HOLD_AB_LONG,
} ink_gesture_type_t;

/* Nút đang giữ khi trigger bắt đầu. */
typedef enum : uint8_t {
    INK_GESTURE_MODIFIER_NONE = 0,
    INK_GESTURE_MODIFIER_A    = INK_GESTURE_BUTTON_A,
    INK_GESTURE_MODIFIER_B    = INK_GESTURE_BUTTON_B,
} ink_gesture_modifier_t;

/* (modifier, trigger); trigger = source, button, type. */
typedef struct {
    ink_gesture_modifier_t modifier;
    ink_gesture_source_t   source;
    ink_gesture_button_t   button;
    ink_gesture_type_t     type;
} ink_gesture_t;

/* ---- Lỗi ---- */

/* Chỉ gồm lỗi thật sự trả về. */
typedef enum : uint8_t {
    INK_GESTURE_OK = 0,
    INK_GESTURE_ERROR_ARGUMENT,
    INK_GESTURE_ERROR_CONFIG, /* ngưỡng sai */
} ink_gesture_error_t;

/* ---- Đầu vào: edge nút ---- */

typedef enum : uint8_t {
    INK_GESTURE_LEVEL_RELEASED = 0,
    INK_GESTURE_LEVEL_PRESSED,
} ink_gesture_level_t;

/* Một edge: level sau edge, time do bên gọi đóng dấu. */
typedef struct {
    ink_gesture_button_t button;
    ink_gesture_level_t  level;
    uint32_t             time_ms;
} ink_gesture_event_t;

/* ---- Đầu ra ---- */

#define INK_GESTURE_OUTPUT_MAX 4

typedef struct {
    uint8_t       count;
    ink_gesture_t items[INK_GESTURE_OUTPUT_MAX];
} ink_gesture_output_t;

/* Gesture đang dở, để system vẽ tiến độ. */
typedef struct {
    ink_gesture_modifier_t modifier;
    ink_gesture_button_t   button;
    ink_gesture_type_t     on_release; /* gesture sẽ ra nếu nhả lúc này */
    bool                   has_next;
    ink_gesture_type_t     next; /* gesture có được nếu giữ tới next_ms */
    uint32_t               start_ms;
    uint32_t               next_ms;
} ink_gesture_progress_t;

/* ---- Cấu hình ---- */

typedef struct {
    uint32_t debounce_ms;
    uint32_t double_ms;
    uint32_t hold_ms;
    uint32_t combo_ms;
    uint32_t ab_short_ms;
    uint32_t ab_long_ms;
} ink_gesture_config_t;

/* Bit mask cho ink_gesture_set_enabled(): kiểu phải chờ, mặc định tắt. */
typedef enum : uint8_t {
    INK_GESTURE_WAIT_NONE     = 0,
    INK_GESTURE_WAIT_DOUBLE   = 1 << 0,
    INK_GESTURE_WAIT_MODIFIER = 1 << 1,
} ink_gesture_wait_mask_t;

/* ---- Trạng thái ---- */

/* Trạng thái một nút, chỉ dùng trong ink_gesture_state_t. */
typedef struct {
    uint8_t  phase;
    uint8_t  wait_mask;
    uint8_t  modifier;         /* modifier chốt lúc bấm */
    bool     stable;           /* level sau debounce; true = bấm */
    bool     raw;              /* level của edge mới nhất */
    bool     locked;           /* trong debounce_ms sau edge đã nhận */
    bool     used_as_modifier; /* đang làm modifier cho nút kia */
    uint32_t lock_until_ms;
    uint32_t pressed_at_ms;
    uint32_t released_at_ms;
} ink_gesture_button_state_t;

/* Bên gọi giữ struct, không đụng trường: check.py chặn truy cập `priv` ngoài
 * khối lõi. */
typedef struct {
    struct {
        ink_gesture_config_t       config;
        ink_gesture_button_state_t buttons[2];
        uint8_t                    mode;
        uint32_t                   ab_pressed_at_ms; /* lúc bấm nút thứ hai của A+B */
    } priv;
} ink_gesture_state_t;

/* ---- API ---- */

/* Trả ERROR_ARGUMENT nếu nullptr, ERROR_CONFIG nếu config sai. */
[[nodiscard]] ink_gesture_error_t ink_gesture_init(ink_gesture_state_t *state, const ink_gesture_config_t *config);

/* Đổi ngưỡng lúc chạy; gesture dở dùng ngưỡng mới ngay. Config sai: trả
 * ERROR_CONFIG, giữ config cũ. */
[[nodiscard]] ink_gesture_error_t ink_gesture_set_config(ink_gesture_state_t *state, const ink_gesture_config_t *config);

[[nodiscard]] ink_gesture_error_t ink_gesture_get_config(const ink_gesture_state_t *state, ink_gesture_config_t *out_config);

/*
 * Bật kiểu phải chờ cho một nút (bit INK_GESTURE_WAIT_*). system gọi khi mở app
 * hoặc đổi scene. Trả ERROR_ARGUMENT nếu button không phải A, B hoặc wait_mask
 * có bit lạ.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_set_enabled(ink_gesture_state_t *state, ink_gesture_button_t button, uint8_t wait_mask);

/*
 * Nhận một edge. Xử lý trước các deadline tới trước time_ms, nên gesture ra
 * đúng thứ tự. out->count có thể là 0. Trả ERROR_ARGUMENT nếu nullptr, button
 * không phải A, B hoặc level lạ.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_feed(ink_gesture_state_t *state, const ink_gesture_event_t *event, ink_gesture_output_t *out);

/* Xử lý các deadline đã tới tính đến now_ms. Gọi khi timer theo
 * ink_gesture_next_deadline() hết. */
[[nodiscard]] ink_gesture_error_t ink_gesture_tick(ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_output_t *out);

/* Deadline gần nhất. Trả false nếu không có gesture dở, không cần timer. */
bool ink_gesture_next_deadline(const ink_gesture_state_t *state, uint32_t *out_ms);

/* Gesture đang giữ dở tính tới now_ms. Chỉ đọc, không phát gesture. Trả false
 * nếu không có gì đang giữ. */
bool ink_gesture_progress(const ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_progress_t *out);
