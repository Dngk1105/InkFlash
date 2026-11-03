/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Phần quyết định của display: so khung mới với khung đang hiện, tìm vùng đổi, giữ nợ xám và cờ viền của từng
 * điểm, chọn cách vẽ, thang dọn khi nghỉ, dựng mặt phẳng cũ cho lượt dọn. C thuần (C23). Khung theo bố cục của
 * interface màn: 1 bit mỗi điểm (1 = trắng), gói theo hàng, điểm trái nhất ở bit cao nhất; mỗi hàng
 * (width + 7) / 8 byte. Bên gọi cấp bộ nhớ trạng thái: width × height byte.
 */
#include <stddef.h>
#include <stdint.h>

#define INK_DISPLAY_POLICY_MAX_DEBT 15
#define INK_DISPLAY_POLICY_NO_WAIT  UINT32_MAX /**< thang dọn không còn bậc nào phải chờ */

/* ---- Lỗi ---- */

typedef enum : uint8_t {
    INK_DISPLAY_POLICY_OK = 0,
    INK_DISPLAY_POLICY_ERROR_ARGUMENT,
} ink_display_policy_error_t;

/* ---- Kiểu ---- */

/** Ý định vẽ của khung. Chỉ thêm vào cuối: app và bench gửi dạng số. */
typedef enum : uint8_t {
    INK_DISPLAY_POLICY_INTENT_AUTO,
    INK_DISPLAY_POLICY_INTENT_TEXT,
    INK_DISPLAY_POLICY_INTENT_UI,
    INK_DISPLAY_POLICY_INTENT_PAGE,
    INK_DISPLAY_POLICY_INTENT_COUNT,
} ink_display_policy_intent_t;

/** Cờ đi kèm khung. */
enum : uint8_t {
    INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP = 1 << 0, /**< không chạy thang dọn tới khung sau không có cờ này */
    INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP  = 1 << 1, /**< màn nền trước khi ngủ: còn nợ thì dọn mạnh */
};

/** Việc với màn. Chỉ thêm vào cuối: bench gửi dạng số. */
typedef enum : uint8_t {
    INK_DISPLAY_POLICY_ACTION_NONE,
    INK_DISPLAY_POLICY_ACTION_DRAW, /**< preset của ý định, window vùng đổi */
    INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG,
    INK_DISPLAY_POLICY_ACTION_CLEAN_FAST,
    INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER,
    INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_NEIGHBOR,
    INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT,
} ink_display_policy_action_t;

/** Mặt phẳng cũ của lần vẽ. */
typedef enum : uint8_t {
    INK_DISPLAY_POLICY_OLD_SHOWN,    /**< khung đang hiện */
    INK_DISPLAY_POLICY_OLD_INVERTED, /**< khung mới đảo bit: mọi điểm bị kéo */
} ink_display_policy_old_plane_t;

/** Toạ độ theo hướng gốc của panel. */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_display_policy_rect_t;

typedef struct {
    uint16_t width; /**< cỡ gốc, từ bảng khả năng của driver */
    uint16_t height;
    uint8_t  x_align;                                  /**< mép x window là bội của số này */
    uint8_t  weights[INK_DISPLAY_POLICY_INTENT_COUNT]; /**< nợ cộng cho mỗi điểm đổi màu; index AUTO không dùng */
    uint8_t  auto_ui_percent;                          /**< AUTO: window nhỏ hơn chừng này % diện tích màn thì là UI */
    uint8_t  debt_soft;                                /**< dọn nhẹ các điểm có nợ từ mức này */
    uint8_t  debt_page;                                /**< PAGE khi nợ lớn nhất tới mức này thì dọn nhanh */
    uint8_t  debt_ceiling;                             /**< trần nợ lúc nghỉ */
    uint8_t  debt_ceiling_max;                         /**< trần lớn nhất khi đang thao tác liên tục */
    uint32_t edge_ms;                                  /**< bậc 1 của thang dọn; cũng là khoảng "đang thao tác"; 0 = bỏ */
    uint32_t light_ms;                                 /**< bậc 2; 0 = bỏ */
    uint32_t strong_ms;                                /**< bậc 3; 0 = bỏ */
} ink_display_policy_config_t;

typedef struct {
    ink_display_policy_intent_t intent;
    uint8_t                     flags; /**< INK_DISPLAY_POLICY_FLAG_* */
} ink_display_policy_request_t;

typedef struct {
    ink_display_policy_action_t    action; /**< NONE, DRAW, CLEAN_STRONG hoặc CLEAN_FAST */
    ink_display_policy_intent_t    intent; /**< DRAW: ý định đã chọn (TEXT, UI, PAGE) */
    ink_display_policy_rect_t      window; /**< cả màn khi dọn; cỡ 0 khi NONE */
    ink_display_policy_old_plane_t old_plane;
} ink_display_policy_decision_t;

typedef struct {
    ink_display_policy_action_t action;  /**< NONE: chờ wait_ms rồi hỏi lại */
    uint32_t                    wait_ms; /**< INK_DISPLAY_POLICY_NO_WAIT khi không còn bậc nào */
} ink_display_policy_idle_t;

typedef struct {
    struct {
        ink_display_policy_config_t config;
        uint8_t                    *pixels; /**< mỗi điểm: bit 0–1 cờ viền, bit 2–5 nợ */
        uint32_t                    flagged_count;
        uint32_t                    last_frame_ms;
        uint8_t                     max_debt;
        uint8_t                     ceiling;
        uint8_t                     stages_done; /**< bit theo bậc của thang dọn trong lần nghỉ này */
        bool                        edge_center_done;
        bool                        defer;
        bool                        has_frame;   /**< last_frame_ms có nghĩa */
        bool                        shown_valid; /**< màn đang hiện đúng khung `shown` bên gọi giữ */
        bool                        after_wake;  /**< `shown` là màn nền vẽ trước khi ngủ */
    } priv;
} ink_display_policy_state_t;

/* ---- API ---- */

/**
 * Khởi tạo; mọi điểm nợ 0, không cờ; lần decide đầu tiên là dọn mạnh.
 * Gọi từ: display, lúc khởi động.
 * @param pixels width × height byte, sống cùng state
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu có con trỏ nullptr hoặc config sai
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_init(ink_display_policy_state_t        *state,
                                                                 const ink_display_policy_config_t *config, uint8_t *pixels);

/**
 * Đổi config cho các lần sau, giữ nợ và cờ. Cỡ màn và x_align phải giữ nguyên.
 * Gọi từ: task màn.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu nullptr, config sai hoặc đổi cỡ màn; giữ config cũ
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_set_config(ink_display_policy_state_t        *state,
                                                                       const ink_display_policy_config_t *config);

/**
 * Chọn cách vẽ `next`, coi như sau đó màn hiện `next`; cập nhật nợ, cờ viền và bắt đầu lại thang dọn.
 * Gọi từ: task màn, mỗi khung.
 * @param shown khung đang hiện; nullptr khi chưa có (lần đầu, hoặc sau invalidate)
 * @param now_ms đồng hồ ms của bên gọi, được quay vòng
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu nullptr, ý định ngoài enum, hoặc shown là nullptr khi màn đang có khung hiện
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_decide(ink_display_policy_state_t *state, const uint8_t *shown,
                                                                   const uint8_t *next, ink_display_policy_request_t request,
                                                                   uint32_t now_ms, ink_display_policy_decision_t *out_decision);

/**
 * Bậc tiếp theo của thang dọn khi không có khung mới.
 * Gọi từ: task màn, sau mỗi khung và sau mỗi lượt dọn.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu nullptr
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_idle(const ink_display_policy_state_t *state, uint32_t now_ms,
                                                                 ink_display_policy_idle_t *out_idle);

/**
 * Dựng mặt phẳng cũ cho lượt dọn `action` (CLEAN_EDGE_CENTER, CLEAN_EDGE_NEIGHBOR, CLEAN_LIGHT, CLEAN_STRONG):
 * chép `shown` rồi đảo bit ở các điểm được dọn (dọn mạnh: mọi điểm). Mặt phẳng mới của lượt dọn là `shown`.
 * Gọi từ: task màn, sau idle.
 * @param out_window bao các điểm được dọn, mép x đã căn; cỡ 0 nếu không có điểm nào
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu nullptr hoặc action không phải lượt dọn khi nghỉ
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_cleanup_plane(const ink_display_policy_state_t *state, const uint8_t *shown,
                                                                          ink_display_policy_action_t action, uint8_t *out_old,
                                                                          ink_display_policy_rect_t *out_window);

/**
 * Lượt dọn `action` đã vẽ xong (hoặc không có điểm nào): xoá cờ và nợ đã dọn, đánh dấu bậc đó xong.
 * Gọi từ: task màn.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu nullptr hoặc action không phải lượt dọn khi nghỉ
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_cleanup_done(ink_display_policy_state_t *state, ink_display_policy_action_t action);

/**
 * Màn không chắc đang hiện gì (driver vẽ lỗi): lần decide sau là dọn mạnh.
 * Gọi từ: task màn. Bỏ qua state nullptr.
 */
void ink_display_policy_invalidate(ink_display_policy_state_t *state);

/**
 * Vừa thức dậy, `shown` của bên gọi là màn nền vẽ trước khi ngủ: lần decide sau vẽ cả màn bằng preset của ý định.
 * Gọi từ: display, lúc khởi động. Bỏ qua state nullptr.
 */
void ink_display_policy_assume_shown(ink_display_policy_state_t *state);

/** Nợ lớn nhất của các điểm; 0 nếu state nullptr. */
uint8_t ink_display_policy_max_debt(const ink_display_policy_state_t *state);

/** Số điểm đang có cờ viền; 0 nếu state nullptr. */
uint32_t ink_display_policy_flagged_count(const ink_display_policy_state_t *state);

/** Trần nợ đang áp dụng; 0 nếu state nullptr. */
uint8_t ink_display_policy_ceiling(const ink_display_policy_state_t *state);

/**
 * Hình chữ nhật bao các điểm khác nhau giữa hai khung, mép x căn theo config.
 * Gọi từ: bất kỳ (firmware test dùng ở đường raw).
 * @return false nếu nullptr hoặc hai khung giống hệt
 */
bool ink_display_policy_changed_window(const ink_display_policy_config_t *config, const uint8_t *shown, const uint8_t *next,
                                       ink_display_policy_rect_t *out_window);
