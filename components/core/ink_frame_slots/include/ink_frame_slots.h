/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Chủ của 3 khung đệm của display. Chỉ giữ chủ của từng khung (theo index), không
 * giữ bộ nhớ ảnh. Không tự khoá: bên gọi giữ lock quanh mỗi hàm, vì task main và
 * task màn cùng gọi. C thuần (C23).
 */
#include <stdint.h>

#define INK_FRAME_SLOTS_COUNT 3
#define INK_FRAME_SLOTS_NONE  (-1) /**< không có khung */

/* ---- Lỗi ---- */

typedef enum : uint8_t {
    INK_FRAME_SLOTS_OK = 0,
    INK_FRAME_SLOTS_ERROR_ARGUMENT,
    INK_FRAME_SLOTS_ERROR_STATE, /**< gọi sai thứ tự: mượn hai lần, giao khung chưa mượn... */
} ink_frame_slots_error_t;

/* ---- Kiểu ---- */

/** Bên mượn phải làm gì với ảnh trong khung vừa mượn. */
typedef enum : uint8_t {
    INK_FRAME_SLOTS_FILL_WHITE, /**< chưa giao khung nào: tô trắng */
    INK_FRAME_SLOTS_FILL_COPY,  /**< chép ảnh từ khung `copy_from` */
    INK_FRAME_SLOTS_FILL_KEEP,  /**< khung lấy lại từ hàng chờ, đã có ảnh mới nhất */
} ink_frame_slots_fill_t;

/** Kết quả của một lần mượn. */
typedef struct {
    int8_t                 frame; /**< index khung vừa mượn */
    ink_frame_slots_fill_t fill;
    int8_t                 copy_from; /**< khung giao gần nhất khi fill = COPY; NONE ở trường hợp khác */
} ink_frame_slots_lease_t;

/** Bên gọi giữ struct nhưng không đụng `priv`. */
typedef struct {
    struct {
        uint8_t  owner[INK_FRAME_SLOTS_COUNT];
        int8_t   borrowed; /* khung task main đang vẽ */
        int8_t   pending;  /* khung đã giao, task màn chưa lấy */
        int8_t   latest;   /* khung giao gần nhất; lần mượn sau chép ảnh từ đây */
        int8_t   shown;    /* khung đang hiện trên màn */
        uint32_t dropped;
    } priv;
} ink_frame_slots_state_t;

/* ---- API ---- */

/**
 * Mọi khung trống, chưa có khung đang hiện.
 * Gọi từ: lúc khởi động, trước khi tạo task màn.
 * @return INK_FRAME_SLOTS_ERROR_ARGUMENT nếu state nullptr
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_init(ink_frame_slots_state_t *state);

/**
 * Khung `frame` thành khung đang hiện và khung giao gần nhất (màn nền khi thức dậy).
 * Gọi từ: lúc khởi động, sau init.
 * @return INK_FRAME_SLOTS_ERROR_ARGUMENT nếu state nullptr hoặc frame ngoài 0..COUNT-1;
 *         INK_FRAME_SLOTS_ERROR_STATE nếu khung đó không trống
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_assume_shown(ink_frame_slots_state_t *state, int8_t frame);

/**
 * Mượn một khung để vẽ. Không còn khung trống (task màn đang giữ hai khung) thì lấy lại
 * khung trong hàng chờ, khung đó tính là bị bỏ.
 * Gọi từ: task main.
 * @return INK_FRAME_SLOTS_ERROR_ARGUMENT nếu có con trỏ nullptr;
 *         INK_FRAME_SLOTS_ERROR_STATE nếu khung mượn trước chưa giao
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_acquire(ink_frame_slots_state_t *state, ink_frame_slots_lease_t *out_lease);

/** Khung task main đang mượn, hoặc INK_FRAME_SLOTS_NONE. */
int8_t ink_frame_slots_borrowed(const ink_frame_slots_state_t *state);

/**
 * Giao khung đang mượn vào hàng chờ. Hàng chờ đã có khung thì khung cũ đó bị bỏ.
 * Gọi từ: task main.
 * @return INK_FRAME_SLOTS_ERROR_ARGUMENT nếu state nullptr;
 *         INK_FRAME_SLOTS_ERROR_STATE nếu chưa mượn khung
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_submit(ink_frame_slots_state_t *state);

/**
 * Lấy khung trong hàng chờ ra để vẽ.
 * Gọi từ: task màn.
 * @return index khung, hoặc INK_FRAME_SLOTS_NONE nếu hàng chờ trống hay state nullptr
 */
int8_t ink_frame_slots_take(ink_frame_slots_state_t *state);

/**
 * Khung vừa lấy ra thành khung đang hiện; khung đang hiện cũ được trả về trống.
 * Gọi từ: task màn, sau khi vẽ xong (kể cả vẽ lỗi) hoặc khi không cần vẽ.
 * @return INK_FRAME_SLOTS_ERROR_ARGUMENT nếu state nullptr hoặc frame ngoài 0..COUNT-1;
 *         INK_FRAME_SLOTS_ERROR_STATE nếu frame không phải khung đã lấy ra
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_show(ink_frame_slots_state_t *state, int8_t frame);

/**
 * Trả khung vừa lấy ra về trống mà không hiện (policy lỗi).
 * Gọi từ: task màn.
 * @return như ink_frame_slots_show()
 */
[[nodiscard]] ink_frame_slots_error_t ink_frame_slots_release(ink_frame_slots_state_t *state, int8_t frame);

/** Khung đang hiện, hoặc INK_FRAME_SLOTS_NONE. */
int8_t ink_frame_slots_shown(const ink_frame_slots_state_t *state);

/** Số khung bị bỏ từ lúc init (giao đè, hoặc bị lấy lại khi mượn). */
uint32_t ink_frame_slots_dropped(const ink_frame_slots_state_t *state);
