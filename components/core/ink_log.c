#include "ink_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#define RING_SIZE 64

static ink_log_record_t s_ring[RING_SIZE];
static uint32_t s_head; /* tổng số bản ghi đã ghi */
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

static const char *const s_names[INK_COMP_COUNT] = {
    [INK_COMP_SYS] = "sys",
    [INK_COMP_BUS] = "bus",
    [INK_COMP_CONFIG] = "config",
    [INK_COMP_DISPLAY] = "display",
    [INK_COMP_BUTTON] = "button",
    [INK_COMP_AUDIO_OUT] = "audio_out",
    [INK_COMP_AUDIO_IN] = "audio_in",
    [INK_COMP_IMU] = "imu",
    [INK_COMP_POWER_HW] = "power_hw",
    [INK_COMP_LED] = "led",
    [INK_COMP_SVC_INPUT] = "svc_input",
    [INK_COMP_SVC_RENDER] = "svc_render",
    [INK_COMP_SVC_STORAGE] = "svc_storage",
    [INK_COMP_SVC_AUDIO] = "svc_audio",
    [INK_COMP_SVC_POWER] = "svc_power",
    [INK_COMP_SVC_TRANSFER] = "svc_transfer",
    [INK_COMP_SVC_SYSTEM] = "svc_system",
    [INK_COMP_APP_MANAGER] = "app_mgr",
    [INK_COMP_APP] = "app",
};

const char *ink_comp_name(ink_comp_t comp)
{
    return (comp < INK_COMP_COUNT && s_names[comp]) ? s_names[comp] : "?";
}

void ink_log_record(ink_comp_t comp, esp_log_level_t level, uint16_t line)
{
    const ink_log_record_t rec = {
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000),
        .comp = (uint8_t)comp,
        .level = (uint8_t)level,
        .line = line,
    };
    portENTER_CRITICAL_SAFE(&s_lock);
    s_ring[s_head % RING_SIZE] = rec;
    s_head++;
    portEXIT_CRITICAL_SAFE(&s_lock);
}

void ink_log_foreach(void (*cb)(const ink_log_record_t *rec, void *arg), void *arg)
{
    const uint32_t head = s_head;
    for (uint32_t i = head > RING_SIZE ? head - RING_SIZE : 0; i < head; i++) {
        ink_log_record_t rec;
        portENTER_CRITICAL_SAFE(&s_lock);
        rec = s_ring[i % RING_SIZE];
        portEXIT_CRITICAL_SAFE(&s_lock);
        cb(&rec, arg);
    }
}
