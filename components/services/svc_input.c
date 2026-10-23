#include "ink_bus.h"
#include "ink_hal_button.h"
#include "ink_log.h"
#include "ink_service.h"
#include "sdkconfig.h"
#include "svc_internal.h"

/*
 * Input: tín hiệu thô (nút; sau này IMU, voice) → cử chỉ → intent ngữ nghĩa.
 * Khung hiện tại: nhấn đơn và nhấn giữ (đo lúc nhả), bảng ánh xạ cố định.
 * GĐ3: nhấn kép, tổ hợp A+B, báo giữ ngay khi đủ thời gian, bảng ánh xạ theo ngữ cảnh do app đặt.
 */
#define MAX_BUTTONS 4

typedef enum { GESTURE_SHORT, GESTURE_LONG, GESTURE_COUNT } gesture_t;

/* Bảng ánh xạ (nút, cử chỉ) → intent. Đổi cách điều khiển chỉ cần sửa bảng này. */
static const ink_intent_t s_map[MAX_BUTTONS][GESTURE_COUNT] = {
    [0] = { [GESTURE_SHORT] = INK_INTENT_NEXT, [GESTURE_LONG] = INK_INTENT_BACK },
    [1] = { [GESTURE_SHORT] = INK_INTENT_SELECT, [GESTURE_LONG] = INK_INTENT_MENU },
};

static uint32_t s_down_ms[MAX_BUTTONS];
static bool s_down[MAX_BUTTONS];

static void emit(ink_intent_t intent)
{
    if (intent == INK_INTENT_NONE) {
        return;
    }
    const ink_intent_evt_t evt = { .intent = intent };
    ink_bus_post(INK_EVT_INTENT, &evt, sizeof(evt));
}

static void on_button(const ink_button_raw_t *b)
{
    if (b->button >= MAX_BUTTONS) {
        return;
    }
    if (b->pressed) {
        s_down[b->button] = true;
        s_down_ms[b->button] = b->time_ms;
        return;
    }
    if (!s_down[b->button]) {
        return; /* nhả mà không thấy bấm: ví dụ lần bấm đánh thức máy */
    }
    s_down[b->button] = false;
    const uint32_t held = b->time_ms - s_down_ms[b->button];
    emit(s_map[b->button][held >= CONFIG_INK_INPUT_LONG_PRESS_MS ? GESTURE_LONG : GESTURE_SHORT]);
}

static void on_msg(const ink_svc_msg_t *msg)
{
    if (msg->type == INK_SVC_MSG_EVENT && msg->id == INK_EVT_BUTTON_RAW) {
        on_button((const ink_button_raw_t *)msg->data);
    }
}

static const int32_t s_events[] = { INK_EVT_BUTTON_RAW, -1 };

static const ink_service_def_t s_def = {
    .name = "svc_input",
    .comp = INK_COMP_SVC_INPUT,
    .stack_size = 3072,
    .priority = 8, /* cao nhất trong các service: bấm là phải phản hồi ngay */
    .events = s_events,
    .on_msg = on_msg,
};

esp_err_t svc_input_start(void)
{
    return ink_service_start(&s_def, NULL);
}
