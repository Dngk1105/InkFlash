/* Thiết kế: docs/design/input.md */
#pragma once

/* Edge nút → gesture. Không đọc đồng hồ: bên gọi tick theo ink_gesture_next_deadline(). */
#include <stdint.h>

/* Enum chỉ thêm giá trị vào cuối: keymap đã lưu dạng số, đổi thứ tự là trỏ sai gesture. Mã nút và modifier là
 * bitmask: mỗi nút một bit, tổ hợp là OR các bit, thêm nút là thêm bit. */

/* Nguồn gesture. */
typedef enum : uint8_t {
    INK_GESTURE_SRC_BTN = 0,
} ink_gesture_source_t;

/* Lỗi riêng, chỉ gồm lỗi thật sự trả về. */
typedef enum : uint8_t {
    INK_GESTURE_OK = 0,
    INK_GESTURE_ERR_ARG,
    INK_GESTURE_ERR_CONFIG, /* ngưỡng sai */
} ink_gesture_err_t;

/* Edge đầu vào chỉ một nút; tổ hợp chỉ có ở gesture đầu ra. */
typedef enum : uint8_t {
    INK_GESTURE_BTN_A  = 1 << 0,
    INK_GESTURE_BTN_B  = 1 << 1,
    INK_GESTURE_BTN_AB = INK_GESTURE_BTN_A | INK_GESTURE_BTN_B,
} ink_gesture_btn_t;

/* Kiểu gesture. */
typedef enum : uint8_t {
    INK_GESTURE_TYPE_PRESS = 0,
    INK_GESTURE_TYPE_DOUBLE_PRESS,
    INK_GESTURE_TYPE_HOLD,
    INK_GESTURE_TYPE_HOLD_AB_SHORT,
    INK_GESTURE_TYPE_HOLD_AB_LONG,
} ink_gesture_type_t;

/* Nút đang giữ khi trigger bắt đầu. */
typedef enum : uint8_t {
    INK_GESTURE_MOD_NONE = 0,
    INK_GESTURE_MOD_A    = INK_GESTURE_BTN_A,
    INK_GESTURE_MOD_B    = INK_GESTURE_BTN_B,
} ink_gesture_mod_t;

/* (modifier, trigger); trigger = src, btn, type. */
typedef struct {
    ink_gesture_mod_t    mod;
    ink_gesture_source_t src;
    ink_gesture_btn_t    btn;
    ink_gesture_type_t   type;
} ink_gesture_t;

typedef enum : uint8_t {
    INK_GESTURE_LEVEL_RELEASED = 0,
    INK_GESTURE_LEVEL_PRESSED,
} ink_gesture_level_t;

/* Một edge: level sau edge, time do bên gọi đóng dấu. */
typedef struct {
    ink_gesture_btn_t   btn;
    ink_gesture_level_t level;
    uint32_t            time_ms;
} ink_gesture_event_t;

/* Bit mask cho ink_gesture_set_enabled(): kiểu phải chờ, mặc định tắt. */
typedef enum : uint8_t {
    INK_GESTURE_WAIT_NONE   = 0,
    INK_GESTURE_WAIT_DOUBLE = 1 << 0,
    INK_GESTURE_WAIT_MOD    = 1 << 1,
} ink_gesture_wait_mask_t;

typedef struct {
    uint32_t debounce_ms;
    uint32_t double_ms;
    uint32_t hold_ms;
    uint32_t combo_ms;
    uint32_t ab_short_ms;
    uint32_t ab_long_ms;
} ink_gesture_config_t;

#define INK_GESTURE_OUT_MAX 4

typedef struct {
    uint8_t       count;
    ink_gesture_t items[INK_GESTURE_OUT_MAX];
} ink_gesture_out_t;

/* Gesture đang dở, để system vẽ tiến độ. */
typedef struct {
    ink_gesture_mod_t  mod;
    ink_gesture_btn_t  btn;
    ink_gesture_type_t on_release; /* gesture sẽ ra nếu nhả lúc này */
    bool               has_next;
    ink_gesture_type_t next; /* gesture có được nếu giữ tới next_ms */
    uint32_t           start_ms;
    uint32_t           next_ms;
} ink_gesture_progress_t;

/* Trạng thái một nút, chỉ dùng trong ink_gesture_state_t. */
typedef struct {
    uint8_t  phase;
    uint8_t  mask;
    uint8_t  mod;         /* modifier chốt lúc bấm */
    bool     stable;      /* level sau debounce; true = bấm */
    bool     raw;         /* level của edge mới nhất */
    bool     locked;      /* trong debounce_ms sau edge đã nhận */
    bool     used_as_mod; /* đang làm modifier cho nút kia */
    uint32_t lock_until;
    uint32_t t_down;
    uint32_t t_up;
} ink_gesture_btn_state_t;

/* Bên gọi giữ struct, không đụng trường: check.py chặn truy cập `priv` ngoài khối lõi. */
typedef struct {
    struct {
        ink_gesture_config_t    cfg;
        ink_gesture_btn_state_t btn[2];
        uint8_t                 mode;
        uint32_t                ab_t; /* lúc bấm nút thứ hai của A+B */
    } priv;
} ink_gesture_state_t;

/* Trả ERR_ARG nếu nullptr, ERR_CONFIG nếu config sai. */
[[nodiscard]] ink_gesture_err_t ink_gesture_init(ink_gesture_state_t *state, const ink_gesture_config_t *cfg);

/* Đổi ngưỡng lúc chạy; gesture dở dùng ngưỡng mới ngay. Config sai: trả ERR_CONFIG, giữ config cũ. */
[[nodiscard]] ink_gesture_err_t ink_gesture_set_config(ink_gesture_state_t *state, const ink_gesture_config_t *cfg);

[[nodiscard]] ink_gesture_err_t ink_gesture_get_config(const ink_gesture_state_t *state, ink_gesture_config_t *out);

/*
 * Bật kiểu phải chờ cho một nút (bit INK_GESTURE_WAIT_*). system gọi khi mở app hoặc đổi scene.
 * Trả ERR_ARG nếu btn không phải A, B hoặc mask có bit lạ.
 */
[[nodiscard]] ink_gesture_err_t ink_gesture_set_enabled(ink_gesture_state_t *state, ink_gesture_btn_t btn,
                                                        uint8_t mask);

/*
 * Nhận một edge. Xử lý trước các deadline tới trước time_ms, nên gesture ra đúng thứ tự. out->count có thể là 0.
 * Trả ERR_ARG nếu nullptr, btn không phải A, B hoặc level lạ.
 */
[[nodiscard]] ink_gesture_err_t ink_gesture_feed(ink_gesture_state_t *state, const ink_gesture_event_t *event,
                                                 ink_gesture_out_t *out);

/* Xử lý các deadline đã tới tính đến now_ms. Gọi khi timer theo ink_gesture_next_deadline() hết. */
[[nodiscard]] ink_gesture_err_t ink_gesture_tick(ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_out_t *out);

/* Deadline gần nhất. Trả false nếu không có gesture dở, không cần timer. */
bool ink_gesture_next_deadline(const ink_gesture_state_t *state, uint32_t *out_ms);

/* Gesture đang giữ dở tính tới now_ms. Chỉ đọc, không phát gesture. Trả false nếu không có gì đang giữ. */
bool ink_gesture_progress(const ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_progress_t *out);
