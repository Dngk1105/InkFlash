/* Thiết kế: docs/design/display/bench.md */
#pragma once

/* Khai báo dùng chung giữa các file của firmware test màn. Giao thức với PC: docs/design/display/bench.md. */
#include "esp_err.h"
#include "ink_display.h"
#include "ink_hal_display.h"
#include "ink_input.h"
#include <stddef.h>
#include <stdint.h>

/* ---- Giao thức ---- */

#define BENCH_SYNC_0        0xA5
#define BENCH_SYNC_1        0x5A
#define BENCH_HEADER_BYTES  6 /* sync 2, type 1, seq 1, length 2 */
#define BENCH_CRC_BYTES     2
#define BENCH_MAX_PAYLOAD   4200 /* UPLOAD: 4 byte đầu + 4096 byte ảnh */
#define BENCH_UPLOAD_HEADER 4    /* slot 1, đệm 1, offset 2 */
#define BENCH_SLOT_COUNT    64
#define BENCH_MAX_SEQUENCE  64

/** Gói PC gửi xuống. */
typedef enum : uint8_t {
    BENCH_HELLO        = 0x01,
    BENCH_SET_PATH     = 0x02,
    BENCH_UPLOAD       = 0x03,
    BENCH_DRAW         = 0x04,
    BENCH_SUBMIT       = 0x05,
    BENCH_PLAY         = 0x06,
    BENCH_STOP         = 0x07,
    BENCH_POWER_OFF    = 0x08,
    BENCH_PROFILE_LOAD = 0x09,
    BENCH_SET_OPTIONS  = 0x0A,
    BENCH_READ_TEMP    = 0x0B,
    BENCH_OTP_DUMP     = 0x0C,
} bench_command_t;

/** Gói máy gửi lên. */
typedef enum : uint8_t {
    BENCH_ACK     = 0x80,
    BENCH_INFO    = 0x81,
    BENCH_DRAWN   = 0x82,
    BENCH_GESTURE = 0x83,
    BENCH_TEMP    = 0x84,
    BENCH_LOG     = 0x85,
    BENCH_OTP     = 0x86,
} bench_event_t;

/** Đường vẽ, giữ qua esp_restart(). */
typedef enum : uint8_t {
    BENCH_PATH_RAW      = 0, /**< task bench gọi thẳng driver */
    BENCH_PATH_PIPELINE = 1, /**< đi qua ink_display như app thật */
} bench_path_t;

/** Nguồn mặt phẳng cũ ở đường raw. */
typedef enum : uint8_t {
    BENCH_OLD_SHOWN    = 0, /**< ảnh đang hiện */
    BENCH_OLD_SLOT     = 1, /**< một slot khác */
    BENCH_OLD_INVERTED = 2, /**< khung mới đảo bit: mọi điểm đổi màu */
    BENCH_OLD_SAME     = 3, /**< bằng khung mới: mọi điểm chạy bảng giữ màu */
} bench_old_source_t;

/** Kiểu window ở đường raw. */
typedef enum : uint8_t {
    BENCH_WINDOW_GIVEN   = 0, /**< x, y, rộng, cao của gói */
    BENCH_WINDOW_WHOLE   = 1,
    BENCH_WINDOW_CHANGED = 2, /**< vùng đổi so với ảnh đang hiện; không đổi thì không vẽ */
} bench_window_kind_t;

/** Cách vẽ của đường raw, lấy từ gói DRAW gần nhất. */
typedef struct {
    uint8_t                       profile; /**< index trong INFO */
    bench_old_source_t            old_source;
    uint8_t                       old_slot;
    ink_hal_display_draw_config_t config;
    bench_window_kind_t           window_kind;
    ink_hal_display_rect_t        window;
} bench_raw_settings_t;

/* ---- Message vào task bench ---- */
/* Hai hàng đợi: gói từ PC (lớn, chỉ task đọc gửi) và sự kiện nhỏ (ISR nút, timer, task màn). */

/** Gói từ PC đã kiểm CRC. */
typedef struct {
    uint8_t  type;
    uint8_t  seq;
    uint16_t length;
    uint8_t  payload[BENCH_MAX_PAYLOAD];
} bench_packet_t;

typedef enum : uint8_t {
    BENCH_EVENT_INPUT,         /**< message của ink_input */
    BENCH_EVENT_SEQUENCE_NEXT, /**< timer giữa hai khung của chuỗi */
    BENCH_EVENT_DRAWN,         /**< ink_display báo vẽ xong (đường pipeline) */
    BENCH_EVENT_POWER_TIMER,   /**< đường raw: hết thời gian chờ thì tắt nguồn màn */
} bench_event_type_t;

typedef struct {
    bench_event_type_t type;

    union {
        ink_input_message_t  input;
        ink_display_report_t report;
    };
} bench_event_message_t;

/** Một lần làm việc với màn, gửi lên PC thành gói DRAWN. Thời gian theo ms, trừ phần driver theo µs. */
typedef struct {
    uint8_t                  slot; /**< slot nguồn của khung */
    esp_err_t                error;
    ink_display_action_t     action;
    bool                     frame;   /**< lần vẽ một khung; false = lượt dọn hoặc tắt nguồn */
    uint8_t                  purpose; /**< 255 ở đường raw */
    uint16_t                 profile_id;
    ink_hal_display_rect_t   window;
    int8_t                   temperature_celsius;
    bool                     temperature_valid;
    uint8_t                  max_debt;
    uint8_t                  ceiling;
    uint32_t                 flagged_count;
    uint32_t                 input_ms; /**< bấm nút → mượn khung; 0 nếu không do nút */
    uint32_t                 queue_ms; /**< giao khung → bắt đầu vẽ */
    uint32_t                 diff_ms;  /**< policy và mặt phẳng cũ */
    ink_hal_display_timing_t driver;
} bench_drawn_t;

/* ---- bench_link.c (mọi task) ---- */

/** Cài driver USB-Serial-JTAG, chuyển log thành gói LOG, tạo task đọc gói đưa vào `packet_queue`. */
esp_err_t bench_link_start(void *packet_queue);

/** Gửi một gói lên PC; an toàn khi gọi từ nhiều task. */
void bench_link_send(uint8_t type, uint8_t seq, const uint8_t *payload, size_t length);

/** Gửi ACK cho gói `seq` kèm mã lỗi. */
void bench_link_ack(uint8_t seq, esp_err_t error);

/* ---- bench_task.c (task bench) ---- */

/** Đường vẽ đã chọn trước lần esp_restart() gần nhất; mặc định raw. */
bench_path_t bench_saved_path(void);

/** Tạo hàng đợi và task bench, khởi động input và đường vẽ. */
esp_err_t bench_task_start(bench_path_t path);

/* ---- bench_draw.c (task bench) ---- */

/** Cấp các slot trong PSRAM và bộ đệm vẽ; khởi động ink_display nếu là đường pipeline (báo vẽ xong vào `event_queue`). */
esp_err_t bench_draw_init(bench_path_t path, const ink_hal_display_device_t *device, void *event_queue);

const ink_hal_display_capabilities_t *bench_draw_capabilities(void);

/** Cài đặt đang dùng của ink_display (đường pipeline), hoặc mặc định (đường raw). */
const ink_display_options_t *bench_draw_options(void);

/** Ghi một đoạn ảnh vào slot. */
esp_err_t bench_draw_upload(uint8_t slot, uint16_t offset, const uint8_t *data, size_t length);

/** Đường raw: vẽ slot bằng driver theo `settings`, chờ xong, điền `out`; out->action NONE nếu vùng đổi trống. */
esp_err_t bench_draw_raw(uint8_t slot, const bench_raw_settings_t *settings, bench_drawn_t *out);

/** Đường raw: tắt nguồn analog của màn. */
esp_err_t bench_draw_power_off(void);

/** Đường pipeline: chép slot vào khung mượn của ink_display rồi giao; kết quả về qua BENCH_EVENT_DRAWN. */
esp_err_t bench_draw_submit(uint8_t slot, uint32_t press_ms, ink_display_intent_t intent, uint8_t flags);

/** Đường pipeline: đổi report của ink_display thành bench_drawn_t cho slot đã giao gần nhất. */
void bench_draw_from_report(const ink_display_report_t *report, bench_drawn_t *out);

/** Đường pipeline: đổi cài đặt của ink_display. */
esp_err_t bench_draw_set_options(const ink_display_options_t *options);

esp_err_t bench_draw_read_temperature(int8_t *out_celsius);
