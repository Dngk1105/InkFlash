#pragma once

#include <stdint.h>
#include "esp_log.h"
#include "ink_comp.h"

/*
 * Log có cấu trúc: mỗi bản ghi = (thời điểm, mã component, event ID).
 * Ghi ra console (ESP_LOG) và RAM ring buffer. TODO: ghi ra flash.
 */
typedef enum {
    INK_LOG_EVT_INIT = 1,
    INK_LOG_EVT_SELF_TEST_FAIL,
    INK_LOG_EVT_RECOVER,
    INK_LOG_EVT_DEGRADED,
    INK_LOG_EVT_STATE,
} ink_log_evt_t;

typedef struct {
    uint32_t time_ms;
    uint8_t comp;
    uint8_t level;
    uint16_t evt;
} ink_log_record_t;

void ink_log_record(ink_comp_t comp, esp_log_level_t level, uint16_t evt);
/* Gọi cb cho từng bản ghi trong ring buffer, cũ trước mới sau. */
void ink_log_foreach(void (*cb)(const ink_log_record_t *rec, void *arg), void *arg);

#define INK_LOG_(lvl, LETTER, comp, evt, fmt, ...)                                   \
    do {                                                                             \
        ink_log_record((comp), (lvl), (evt));                                        \
        ESP_LOG##LETTER(ink_comp_name(comp), "[%u] " fmt, (unsigned)(evt), ##__VA_ARGS__); \
    } while (0)

#define INK_LOGE(comp, evt, fmt, ...) INK_LOG_(ESP_LOG_ERROR, E, comp, evt, fmt, ##__VA_ARGS__)
#define INK_LOGW(comp, evt, fmt, ...) INK_LOG_(ESP_LOG_WARN, W, comp, evt, fmt, ##__VA_ARGS__)
#define INK_LOGI(comp, evt, fmt, ...) INK_LOG_(ESP_LOG_INFO, I, comp, evt, fmt, ##__VA_ARGS__)
