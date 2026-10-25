/* Thiết kế: docs/design/code-structure.md */
#pragma once

/*
 * Mẫu khối C thuần (C23): nhận sự kiện, trả về hành động. Không include gì của ESP-IDF hay FreeRTOS.
 * Trạng thái nằm trong struct do bên gọi giữ (vỏ giữ nó dạng biến static), nên test trên PC tạo được nhiều bản.
 * Ví dụ này đếm sự kiện TICK; đủ `limit` lần thì trả hành động NOTIFY rồi đếm lại.
 */
#include <stdint.h>

/* Lỗi riêng của khối. Chỉ gồm lỗi khối thật sự trả về. */
typedef enum : uint8_t {
    INK_EXAMPLE_OK = 0,
    INK_EXAMPLE_ERR_ARG,
} ink_example_err_t;

/* Enum có kiểu nền cố định 1 byte: kích thước không đổi khi lưu xuống flash hay RTC memory. */
typedef enum : uint8_t {
    INK_EXAMPLE_EVENT_TICK,
    INK_EXAMPLE_EVENT_RESET,
} ink_example_event_type_t;

typedef struct {
    ink_example_event_type_t type;
    uint32_t time_ms; /* thời điểm do bên gọi truyền vào; khối không tự đọc đồng hồ */
} ink_example_event_t;

typedef enum : uint8_t {
    INK_EXAMPLE_ACTION_NONE,
    INK_EXAMPLE_ACTION_NOTIFY,
} ink_example_action_t;

/* Bên gọi giữ struct nhưng không đụng trường bên trong: mọi trường nằm trong `priv`, tools/check.py chặn truy cập
 * `priv` ngoài thư mục khối lõi. */
typedef struct {
    struct {
        uint32_t count;
        uint32_t limit;
    } priv;
} ink_example_state_t;

/*
 * Khởi tạo trạng thái.
 * Trả INK_EXAMPLE_ERR_ARG nếu `state` là nullptr hoặc `limit` bằng 0.
 */
[[nodiscard]] ink_example_err_t ink_example_init(ink_example_state_t *state, uint32_t limit);

/*
 * Xử lý một sự kiện, ghi hành động cần làm vào `out_action`.
 * Trả INK_EXAMPLE_ERR_ARG nếu có con trỏ nullptr hoặc loại sự kiện không hợp lệ.
 */
[[nodiscard]] ink_example_err_t ink_example_handle(ink_example_state_t *state, const ink_example_event_t *event,
                                                   ink_example_action_t *out_action);
