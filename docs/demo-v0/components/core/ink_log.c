#include "ink_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#define RING_SIZE 128

static ink_log_record_t s_ring[RING_SIZE];
static uint32_t s_head; /* tổng số bản ghi đã ghi */
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

static const char *const s_comp_names[INK_COMP_COUNT] = {
    [INK_COMP_SYS] = "sys",
    [INK_COMP_BUS] = "bus",
    [INK_COMP_CONFIG] = "config",
    [INK_COMP_BUTTONS] = "buttons",
    [INK_COMP_EPD] = "epd",
    [INK_COMP_IMU] = "imu",
    [INK_COMP_CHARGER] = "charger",
    [INK_COMP_AUDIO_HW] = "audio_hw",
    [INK_COMP_SD] = "sd",
    [INK_COMP_LED] = "led",
    [INK_COMP_SVC_POWER] = "svc_power",
    [INK_COMP_SVC_GUI] = "svc_gui",
    [INK_COMP_SVC_STORAGE] = "svc_storage",
    [INK_COMP_SVC_AUDIO] = "svc_audio",
    [INK_COMP_SVC_AI] = "svc_ai",
    [INK_COMP_SVC_GESTURE] = "svc_gesture",
    [INK_COMP_REMOTE] = "remote",
};

const char *ink_comp_name(ink_comp_t comp)
{
    return (comp < INK_COMP_COUNT && s_comp_names[comp]) ? s_comp_names[comp] : "?";
}

void ink_log_record(ink_comp_t comp, esp_log_level_t level, uint16_t evt)
{
    ink_log_record_t rec = {
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000),
        .comp = (uint8_t)comp,
        .level = (uint8_t)level,
        .evt = evt,
    };
    portENTER_CRITICAL_SAFE(&s_lock);
    s_ring[s_head % RING_SIZE] = rec;
    s_head++;
    portEXIT_CRITICAL_SAFE(&s_lock);
}

void ink_log_foreach(void (*cb)(const ink_log_record_t *rec, void *arg), void *arg)
{
    uint32_t head = s_head;
    uint32_t start = head > RING_SIZE ? head - RING_SIZE : 0;
    for (uint32_t i = start; i < head; i++) {
        ink_log_record_t rec;
        portENTER_CRITICAL_SAFE(&s_lock);
        rec = s_ring[i % RING_SIZE];
        portEXIT_CRITICAL_SAFE(&s_lock);
        cb(&rec, arg);
    }
}
