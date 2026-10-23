#include "ink_hal_imu.h"
#include "ink_log.h"
#include "ink_service.h"

/*
 * Cử chỉ chỉ chạy khi máy thức, và chỉ dựa vào HAL IMU (mg, hệ trục thiết bị).
 * Đổi chip IMU hay đổi chân thì file này không đổi.
 * TODO: tap, xoay màn.
 */
#define TICK_MS          50
#define FACE_DOWN_Z_MG   (-800) /* z < ngưỡng: màn hình úp xuống */
#define FACE_UP_Z_MG     (-500) /* z > ngưỡng: coi như đã lật lại (trễ, tránh nhấp nháy) */
#define FACE_DOWN_TICKS  (1000 / TICK_MS)

static int s_down_ticks;
static bool s_fired;

static void on_tick(void)
{
    ink_accel_t a;
    if (ink_imu_read_accel(&a) != ESP_OK) {
        return; /* không có IMU hoặc đọc lỗi (health đã ghi nhận): bỏ qua cử chỉ */
    }
    if (a.z_mg < FACE_DOWN_Z_MG) {
        if (++s_down_ticks >= FACE_DOWN_TICKS && !s_fired) {
            s_fired = true;
            INK_LOGI(INK_COMP_SVC_GESTURE, INK_LOG_EVT_STATE, "face down -> sleep");
            ink_bus_post(INK_EVT_SLEEP_REQUEST, NULL, 0);
        }
    } else if (a.z_mg > FACE_UP_Z_MG) {
        s_down_ticks = 0;
        s_fired = false;
    }
}

const ink_service_def_t svc_gesture_def = {
    .name = "svc_gesture",
    .comp = INK_COMP_SVC_GESTURE,
    .stack_size = 3072,
    .priority = 3,
    .tick_ms = TICK_MS,
    .on_tick = on_tick,
};
