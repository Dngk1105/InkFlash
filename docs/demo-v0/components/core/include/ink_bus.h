#pragma once

#include <stddef.h>
#include "esp_err.h"
#include "esp_event.h"
#include "ink_comp.h"

/*
 * Event bus: các service chỉ nói chuyện với nhau qua đây.
 * Bọc esp_event với một event loop riêng (có task riêng), không dùng default loop.
 */
ESP_EVENT_DECLARE_BASE(INK_EVT);

typedef enum {
    INK_EVT_BUTTON,          /* data: ink_button_evt_t */
    INK_EVT_SLEEP_REQUEST,   /* yêu cầu đi ngủ (A+B 2s, hết giờ chờ, úp mặt) */
    INK_EVT_SHIP_REQUEST,    /* yêu cầu ship mode (A+B 8s) */
    INK_EVT_WAKE,            /* vừa thức dậy từ deep sleep */
    INK_EVT_DEVICE_DEGRADED, /* data: ink_comp_t */
    INK_EVT_DEMO_CMD,        /* data: uint8_t, ký tự lệnh ('1', '2', '3'), chỉ dùng cho svc_demo */
} ink_evt_id_t;

typedef enum { INK_BTN_A = 0, INK_BTN_B = 1 } ink_button_t;
typedef enum { INK_BTN_PRESS, INK_BTN_RELEASE } ink_button_action_t;

typedef struct {
    ink_button_t button;
    ink_button_action_t action;
} ink_button_evt_t;

/* Kích thước dữ liệu của event id (0 nếu không có dữ liệu). Thêm event mới thì sửa cả hàm này. */
size_t ink_bus_evt_size(int32_t id);

esp_err_t ink_bus_init(void);
esp_err_t ink_bus_post(ink_evt_id_t id, const void *data, size_t size);
esp_err_t ink_bus_post_from_isr(ink_evt_id_t id, const void *data, size_t size);
/* id = ESP_EVENT_ANY_ID để nhận mọi event của INK_EVT. */
esp_err_t ink_bus_subscribe(int32_t id, esp_event_handler_t handler, void *arg);
