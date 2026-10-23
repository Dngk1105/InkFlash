#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "ink_comp.h"

/*
 * Event bus: thông báo giữa các khối (pub/sub), bọc esp_event với một event loop riêng.
 * Xem docs/adr/0002-event-bus-esp-event.md.
 *
 * Quy tắc:
 *  - Bus chỉ để THÔNG BÁO ("đã xảy ra X"). Ra LỆNH cho service thì gọi hàm API của service đó.
 *  - Mọi handler chạy tuần tự trong task của loop: handler phải ngắn, chỉ chép sự kiện vào queue của mình.
 *  - Dữ liệu sự kiện được esp_event chép vào heap; giữ nhỏ (≤ 32 byte).
 */
ESP_EVENT_DECLARE_BASE(INK_EVT);

typedef enum {
    INK_EVT_BUTTON_RAW,   /* data: ink_button_raw_t — từ HAL nút, chưa qua nhận diện cử chỉ */
    INK_EVT_INTENT,       /* data: ink_intent_evt_t — ý định ngữ nghĩa, app chỉ nghe sự kiện này */
    INK_EVT_POWER_STATE,  /* data: ink_power_state_evt_t */
    INK_EVT_RENDER_DONE,  /* data: ink_render_done_t */
    INK_EVT_STORAGE_READY,/* không data */
    INK_EVT_DEVICE_DEGRADED, /* data: ink_comp_t */
} ink_evt_id_t;

typedef struct {
    uint8_t button;    /* chỉ số nút do BSP đặt (0 = A, 1 = B, ...) */
    uint8_t pressed;   /* 1 = vừa bấm, 0 = vừa nhả */
    uint32_t time_ms;  /* thời điểm theo esp_timer, dùng cho nhận diện cử chỉ */
} ink_button_raw_t;

/* Ý định ngữ nghĩa. App không bao giờ biết nút nào hay cử chỉ nào tạo ra intent. */
typedef enum {
    INK_INTENT_NONE = 0,
    INK_INTENT_NEXT,
    INK_INTENT_BACK,
    INK_INTENT_SELECT,
    INK_INTENT_MENU,
    INK_INTENT_FLIP,
    INK_INTENT_GRADE_AGAIN,
    INK_INTENT_GRADE_HARD,
    INK_INTENT_GRADE_GOOD,
    INK_INTENT_GRADE_EASY,
    INK_INTENT_PLAY_AUDIO,
    INK_INTENT_SLEEP,
} ink_intent_t;

typedef struct {
    ink_intent_t intent;
} ink_intent_evt_t;

typedef enum {
    INK_POWER_ACTIVE,
    INK_POWER_IDLE,
    INK_POWER_GOING_TO_SLEEP, /* service nhận được thì lưu trạng thái, tắt phần của mình */
} ink_power_state_t;

typedef struct {
    ink_power_state_t state;
} ink_power_state_evt_t;

typedef struct {
    uint32_t request_id;
    uint32_t duration_ms;
    esp_err_t result;
} ink_render_done_t;

/* Kích thước dữ liệu của mỗi loại sự kiện (esp_event không đưa kích thước cho handler). Thêm sự kiện thì sửa cả hàm này. */
size_t ink_bus_evt_size(int32_t id);

esp_err_t ink_bus_init(void);
esp_err_t ink_bus_post(ink_evt_id_t id, const void *data, size_t size);
esp_err_t ink_bus_post_from_isr(ink_evt_id_t id, const void *data, size_t size);
/* id = ESP_EVENT_ANY_ID để nhận mọi sự kiện. `out_instance` dùng để huỷ đăng ký (có thể NULL). */
esp_err_t ink_bus_subscribe(int32_t id, esp_event_handler_t handler, void *arg,
                            esp_event_handler_instance_t *out_instance);
esp_err_t ink_bus_unsubscribe(int32_t id, esp_event_handler_instance_t instance);
/* Số lần post thất bại (queue của loop đầy) kể từ lúc khởi động. */
uint32_t ink_bus_drop_count(void);
