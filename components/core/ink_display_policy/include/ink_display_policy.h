/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Phần quyết định của display: so khung mới với khung đang hiện, tìm vùng đổi,
 * chọn partial hay full, dựng mặt phẳng cũ để đẩy lại viền. C thuần (C23). Khung
 * theo bố cục của interface màn: 1 bit mỗi điểm, gói theo hàng, điểm trái nhất
 * ở bit cao nhất; mỗi hàng (width + 7) / 8 byte.
 */
#include <stdint.h>

#define INK_DISPLAY_POLICY_MAX_EDGE_MARGIN 7

/* ---- Lỗi ---- */

typedef enum : uint8_t {
    INK_DISPLAY_POLICY_OK = 0,
    INK_DISPLAY_POLICY_ERROR_ARGUMENT,
} ink_display_policy_error_t;

/* ---- Kiểu ---- */

typedef enum : uint8_t {
    INK_DISPLAY_POLICY_MODE_NONE, /**< khung giống hệt, không vẽ */
    INK_DISPLAY_POLICY_MODE_PARTIAL,
    INK_DISPLAY_POLICY_MODE_FULL,
} ink_display_policy_mode_t;

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
    uint8_t  x_align;              /**< mép x vùng partial là bội của số này */
    uint16_t partials_before_full; /**< ngưỡng bóng mờ: số lần partial liên tiếp */
    uint8_t  max_partial_percent;  /**< vùng đổi lớn hơn chừng này % diện tích màn thì full; 100 = tắt */
    uint8_t  edge_margin;          /**< đẩy lại viền rộng chừng này điểm quanh vùng đổi; 0 = tắt */
} ink_display_policy_config_t;

typedef struct {
    ink_display_policy_mode_t mode;
    ink_display_policy_rect_t window; /**< cả màn khi FULL; cỡ 0 khi NONE */
} ink_display_policy_decision_t;

typedef struct {
    struct {
        ink_display_policy_config_t config;
        uint16_t                    partial_count;
        bool                        shown_valid; /**< màn đang hiện đúng khung `shown` bên gọi giữ */
        bool                        after_wake;  /**< `shown` là màn nền vẽ trước khi ngủ; lần vẽ tới là partial cả màn */
    } priv;
} ink_display_policy_state_t;

/* ---- API ---- */

/**
 * Khởi tạo; lần decide đầu tiên luôn FULL.
 * Gọi từ: display, lúc khởi động.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu có con trỏ nullptr, cỡ màn hoặc x_align bằng 0,
 *         partials_before_full bằng 0, max_partial_percent ngoài 1–100, edge_margin quá INK_DISPLAY_POLICY_MAX_EDGE_MARGIN
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_init(ink_display_policy_state_t        *state,
                                                                 const ink_display_policy_config_t *config);

/**
 * Chọn cách vẽ `next`, coi như sau đó màn hiện `next`.
 * Gọi từ: task màn, mỗi khung.
 * @param shown khung đang hiện; nullptr khi chưa có (lần đầu, hoặc sau invalidate)
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu state, next hoặc out_decision là nullptr,
 *         hoặc shown là nullptr khi màn đang có khung hiện
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_decide(ink_display_policy_state_t *state, const uint8_t *shown,
                                                                   const uint8_t *next, ink_display_policy_decision_t *out_decision);

/**
 * Màn không chắc đang hiện gì (driver vẽ lỗi): lần decide sau là FULL.
 * Gọi từ: task màn. Bỏ qua state nullptr.
 */
void ink_display_policy_invalidate(ink_display_policy_state_t *state);

/**
 * Vừa thức dậy, `shown` của bên gọi là màn nền vẽ trước khi ngủ: lần decide sau là partial cả màn nếu có đổi.
 * Gọi từ: display, lúc khởi động. Bỏ qua state nullptr.
 */
void ink_display_policy_assume_shown(ink_display_policy_state_t *state);

/**
 * Đổi độ rộng đẩy lại viền cho các lần decide sau.
 * Gọi từ: task màn.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu state nullptr hoặc margin quá INK_DISPLAY_POLICY_MAX_EDGE_MARGIN
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_set_edge_margin(ink_display_policy_state_t *state, uint8_t margin);

/**
 * Đổi ngưỡng bóng mờ (số lần partial liên tiếp trước khi full) cho các lần decide sau; số lần đã đếm giữ nguyên.
 * Gọi từ: task màn.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu state nullptr hoặc count bằng 0
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_set_partials_before_full(ink_display_policy_state_t *state, uint16_t count);

/**
 * Dựng mặt phẳng cũ để đẩy lại viền: chép `shown`, rồi đảo bit cũ của các điểm không đổi
 * nằm trong edge_margin điểm quanh một điểm đổi. Controller coi các điểm đó là đổi màu nên đẩy chúng về đúng
 * màu đang có. Chỉ ghi các byte phủ window.
 * Gọi từ: task màn, sau decide.
 * @return INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu có con trỏ nullptr hoặc window ra ngoài màn
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_edge_plane(const ink_display_policy_state_t *state, const uint8_t *shown,
                                                                       const uint8_t *next, ink_display_policy_rect_t window,
                                                                       uint8_t *out_old);
