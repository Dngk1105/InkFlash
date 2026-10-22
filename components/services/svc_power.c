#include "esp_ota_ops.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "ink_device.h"
#include "ink_hal_power.h"
#include "ink_log.h"
#include "ink_service.h"

#define HOLD_SLEEP_MS       2000
#define HOLD_SHIP_MS        8000
#define DEFAULT_AUTO_SLEEP_S 60

static bool s_pressed[2];
static int64_t s_both_since_us;
static bool s_sleep_triggered;
static bool s_ship_triggered;
static int64_t s_last_activity_us;
static uint32_t s_auto_sleep_sec = DEFAULT_AUTO_SLEEP_S;

/* Chưa xác nhận firmware OTA mới thì cấm deep sleep (để rollback còn chạy được). */
static bool ota_pending_verify(void)
{
    esp_ota_img_states_t state;
    const esp_partition_t *running = esp_ota_get_running_partition();
    return esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY;
}

static void on_start(void)
{
    s_last_activity_us = esp_timer_get_time();
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        uint64_t status = esp_sleep_get_ext1_wakeup_status();
        INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_INIT, "woke from deep sleep (EXT1 status=0x%llx)",
                 (unsigned long long)status);
        ink_bus_post(INK_EVT_WAKE, NULL, 0);
    } else if (cause != ESP_SLEEP_WAKEUP_UNDEFINED) {
        INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_INIT, "woke with cause: %d", (int)cause);
        ink_bus_post(INK_EVT_WAKE, NULL, 0);
    } else {
        INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_INIT, "power on reset (fresh boot)");
    }
}

static void handle_button(const ink_button_evt_t *b)
{
    s_last_activity_us = esp_timer_get_time();

    bool was_both = s_pressed[INK_BTN_A] && s_pressed[INK_BTN_B];
    s_pressed[b->button] = (b->action == INK_BTN_PRESS);
    bool is_both = s_pressed[INK_BTN_A] && s_pressed[INK_BTN_B];

    if (!was_both && is_both) {
        s_both_since_us = esp_timer_get_time();
        s_sleep_triggered = false;
        s_ship_triggered = false;
    } else if (!is_both) {
        s_both_since_us = 0;
        s_sleep_triggered = false;
        s_ship_triggered = false;
    }
}

static void on_tick(void)
{
    int64_t now_us = esp_timer_get_time();

    /* Bấm giữ đồng thời A+B: phản hồi ngay khi chạm mốc thời gian */
    if (s_both_since_us != 0) {
        int64_t held_ms = (now_us - s_both_since_us) / 1000;
        if (held_ms >= HOLD_SHIP_MS && !s_ship_triggered) {
            s_ship_triggered = true;
            INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "A+B held 8s -> ship mode");
            ink_bus_post(INK_EVT_SHIP_REQUEST, NULL, 0);
        } else if (held_ms >= HOLD_SLEEP_MS && !s_sleep_triggered) {
            s_sleep_triggered = true;
            INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "A+B held 2s -> sleep");
            ink_bus_post(INK_EVT_SLEEP_REQUEST, NULL, 0);
        }
    }

    /* Tự ngủ khi không có thao tác (inactivity auto-sleep) */
    if (s_auto_sleep_sec > 0 && !s_pressed[INK_BTN_A] && !s_pressed[INK_BTN_B]) {
        uint32_t idle_sec = (uint32_t)((now_us - s_last_activity_us) / 1000000);
        if (idle_sec >= s_auto_sleep_sec) {
            INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "auto-sleep timeout (%lu s)", (unsigned long)idle_sec);
            s_last_activity_us = now_us; /* tránh kích hoạt lặp lại */
            ink_bus_post(INK_EVT_SLEEP_REQUEST, NULL, 0);
        }
    }
}

static void go_to_sleep(void)
{
    if (ota_pending_verify()) {
        INK_LOGW(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "sleep blocked: OTA pending verify");
        return;
    }

    INK_LOGI(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "preparing deep sleep");
    ink_device_suspend_all();

    for (int r = 0; r < INK_RAIL_COUNT; r++) {
        ink_power_set_rail((ink_rail_t)r, false);
    }

    /* Driver thật sẽ không quay về từ hàm này */
    esp_err_t err = ink_power_deep_sleep();
    INK_LOGW(INK_COMP_SVC_POWER, INK_LOG_EVT_STATE, "deep sleep returned: %s", esp_err_to_name(err));

    /* Nếu là driver giả lập hoặc ngủ lỗi, khôi phục lại hệ thống để tiếp tục */
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
    .on_start = on_start,
    .on_event = on_event,
    .tick_ms = 100, /* 100ms kiểm tra giữ nút và đếm thời gian tự ngủ */
    .on_tick = on_tick,
};
