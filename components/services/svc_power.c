#include "esp_timer.h"
#include "ink_bus.h"
#include "ink_log.h"
#include "ink_service.h"
#include "sdkconfig.h"
#include "svc_internal.h"

/*
 * Power: quyết định KHI NÀO ngủ. Khung hiện tại chỉ có hẹn giờ không thao tác.
 * GĐ6: ACTIVE → IDLE (light sleep qua esp_pm) → GOING_TO_SLEEP (phát sự kiện, chờ service lưu trạng thái,
 * vẽ màn nền) → deep sleep qua HAL power; cấm ngủ khi firmware OTA chưa xác nhận; xử lý lần bấm đánh thức.
 */
static int64_t s_last_activity_us;
static bool s_sleep_logged;

static void on_msg(const ink_svc_msg_t *msg)
{
    if (msg->type == INK_SVC_MSG_EVENT) {
        s_last_activity_us = esp_timer_get_time();
        s_sleep_logged = false;
    }
}

static void on_tick(void)
{
    const int64_t idle_s = (esp_timer_get_time() - s_last_activity_us) / 1000000;
    if (idle_s >= CONFIG_INK_POWER_IDLE_SLEEP_S && !s_sleep_logged) {
        s_sleep_logged = true;
        INK_LOGI(INK_COMP_SVC_POWER, "khong thao tac %llds: se ngu sau o GD6", (long long)idle_s);
    }
}

static void on_start(void)
{
    s_last_activity_us = esp_timer_get_time();
}

static const int32_t s_events[] = { INK_EVT_BUTTON_RAW, INK_EVT_INTENT, -1 };

static const ink_service_def_t s_def = {
    .name = "svc_power",
    .comp = INK_COMP_SVC_POWER,
    .stack_size = 3072,
    .priority = 7,
    .events = s_events,
    .tick_ms = 1000,
    .on_start = on_start,
    .on_msg = on_msg,
    .on_tick = on_tick,
};

esp_err_t svc_power_start(void)
{
    return ink_service_start(&s_def, NULL);
}
