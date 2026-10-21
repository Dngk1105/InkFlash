#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "ink_device.h"
#include "ink_hal_power.h"
#include "ink_log.h"
#include "ink_service.h"

#define HOLD_SLEEP_MS 2000
#define HOLD_SHIP_MS  8000

static bool s_pressed[2];
static int64_t s_both_since_us; /* 0 = A+B không đang cùng giữ */

/* Chưa xác nhận firmware OTA mới thì cấm deep sleep (để rollback còn chạy được). */
static bool ota_pending_verify(void)
{
    esp_ota_img_states_t state;
    const esp_partition_t *running = esp_ota_get_running_partition();
    return esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY;
}

/* A+B giữ 2s = ngủ, 8s = ship mode. Tính khi thả nút. TODO: báo ngay khi đủ 8s, không chờ thả. */
static void handle_button(const ink_button_evt_t *b)
{
    bool was_both = s_pressed[INK_BTN_A] && s_pressed[INK_BTN_B];
    s_pressed[b->button] = (b->action == INK_BTN_PRESS);
    bool is_both = s_pressed[INK_BTN_A] && s_pressed[INK_BTN_B];

    if (!was_both && is_both) {
        s_both_since_us = esp_timer_get_time();
    } else if (was_both && !is_both && s_both_since_us) {
        int64_t held_ms = (esp_timer_get_time() - s_both_since_us) / 1000;
        s_both_since_us = 0;
        if (held_ms >= HOLD_SHIP_MS) {
            ink_bus_post(INK_EVT_SHIP_REQUEST, NULL, 0);
        } else if (held_ms >= HOLD_SLEEP_MS) {
            ink_bus_post(INK_EVT_SLEEP_REQUEST, NULL, 0);
        }
    }
    /* TODO: đặt lại bộ đếm tự ngủ. */
}

static void go_to_sleep(void)
{
    if (ota_pending_verify()) {
        INK_LOGW(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "sleep blocked: OTA pending verify");
        return;
    }
    /* TODO: GUI vẽ màn nền (số thẻ cần ôn, pin) trước khi ngủ. */
    ink_device_suspend_all();
    for (int r = 0; r < INK_RAIL_COUNT; r++) {
        ink_power_set_rail((ink_rail_t)r, false);
    }
    /* Driver thật không quay về. Còn quay về nghĩa là driver giả lập hoặc ngủ thất bại: bật lại để máy chạy tiếp. */
    esp_err_t err = ink_power_deep_sleep();
    INK_LOGW(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "deep sleep returned: %s", esp_err_to_name(err));
    for (int r = 0; r < INK_RAIL_COUNT; r++) {
        ink_power_set_rail((ink_rail_t)r, true);
    }
    ink_device_resume_all();
}

static void on_event(const ink_svc_msg_t *msg)
{
    switch (msg->id) {
    case INK_EVT_BUTTON:
        handle_button((const ink_button_evt_t *)msg->data);
        break;
    case INK_EVT_SLEEP_REQUEST:
        go_to_sleep();
        break;
    case INK_EVT_SHIP_REQUEST:
        INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "ship mode: %s", esp_err_to_name(ink_power_ship_mode()));
        break;
    default:
        break;
    }
}

const ink_service_def_t svc_power_def = {
    .name = "svc_power",
    .comp = INK_COMP_SVC_POWER,
    .stack_size = 3072,
    .priority = 6,
    .on_event = on_event,
};
