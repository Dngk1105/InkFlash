/* Thiết kế: docs/design/display-pipeline.md */
#pragma once

/*
 * Phần quyết định của display: so khung mới với khung đang hiện, tìm vùng đổi, chọn partial hay full.
 * C thuần (C23). Khung theo bố cục của interface màn: 1 bit mỗi điểm, gói theo hàng, điểm trái nhất ở bit cao nhất;
 * mỗi hàng (width + 7) / 8 byte.
 */
#include <stdint.h>

/* ---- Lỗi ---- */

typedef enum : uint8_t {
    INK_DISPLAY_POLICY_OK = 0,
    INK_DISPLAY_POLICY_ERROR_ARGUMENT,
} ink_display_policy_error_t;

/* ---- Kiểu ---- */

typedef enum : uint8_t {
    INK_DISPLAY_POLICY_MODE_NONE, /* khung giống hệt, không vẽ */
    INK_DISPLAY_POLICY_MODE_PARTIAL,
    INK_DISPLAY_POLICY_MODE_FULL,
} ink_display_policy_mode_t;

/* Toạ độ theo hướng gốc của panel. */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_display_policy_rect_t;

typedef struct {
    uint16_t width; /* cỡ gốc, từ bảng khả năng của driver */
    uint16_t height;
    bool     has_partial;
    uint8_t  x_align;              /* mép x vùng partial là bội của số này */
    uint16_t partials_before_full; /* ngưỡng bóng mờ: số lần partial liên tiếp */
    uint8_t  max_partial_percent;  /* vùng đổi lớn hơn chừng này % diện tích màn thì full; 100 = tắt */
} ink_display_policy_config_t;

typedef struct {
    ink_display_policy_mode_t mode;
    ink_display_policy_rect_t window; /* cả màn khi FULL; cỡ 0 khi NONE */
} ink_display_policy_decision_t;

typedef struct {
    struct {
        ink_display_policy_config_t config;
        uint16_t                    partial_count;
        bool                        shown_valid; /* màn đang hiện đúng khung `shown` bên gọi giữ */
    } priv;
} ink_display_policy_state_t;

/* ---- API ---- */

/*
 * Khởi tạo; lần decide đầu tiên luôn FULL.
 * Trả INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu có con trỏ nullptr, cỡ màn hoặc x_align bằng 0,
 * partials_before_full bằng 0, max_partial_percent bằng 0 hoặc lớn hơn 100.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_init(ink_display_policy_state_t        *state,
                                                                 const ink_display_policy_config_t *config);

/*
 * Chọn cách vẽ `next`, coi như sau đó màn hiện `next`. `shown` là khung đang hiện; được là nullptr khi lần trước chưa
 * có khung hiện (lần đầu, hoặc sau invalidate).
 * Trả INK_DISPLAY_POLICY_ERROR_ARGUMENT nếu state, next hoặc out_decision là nullptr, hoặc shown là nullptr khi
 * màn đang có khung hiện.
 */
[[nodiscard]] ink_display_policy_error_t ink_display_policy_decide(ink_display_policy_state_t *state,
                                                                   const uint8_t *shown, const uint8_t *next,
                                                                   ink_display_policy_decision_t *out_decision);

/* Driver vẽ lỗi, màn không chắc hiện gì: lần decide sau là FULL. Bỏ qua state nullptr. */
void ink_display_policy_invalidate(ink_display_policy_state_t *state);
