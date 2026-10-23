#pragma once

#include <stdint.h>
#include "esp_log.h"
#include "ink_comp.h"

/*
 * Log theo component: in ra console qua ESP_LOG (tag = tên component) và ghi lỗi/cảnh báo vào RAM ring buffer
 * để lệnh console `log` xem lại sau. TODO (GĐ sau): ghi ring buffer ra flash khi crash.
 */
typedef struct {
    uint32_t time_ms;
    uint8_t comp;
    uint8_t level;
    uint16_t line;
} ink_log_record_t;

void ink_log_record(ink_comp_t comp, esp_log_level_t level, uint16_t line);
void ink_log_foreach(void (*cb)(const ink_log_record_t *rec, void *arg), void *arg);

#define INK_LOGE(comp, fmt, ...)                                         \
    do {                                                                 \
        ink_log_record((comp), ESP_LOG_ERROR, __LINE__);                 \
        ESP_LOGE(ink_comp_name(comp), fmt, ##__VA_ARGS__);               \
    } while (0)
#define INK_LOGW(comp, fmt, ...)                                         \
    do {                                                                 \
        ink_log_record((comp), ESP_LOG_WARN, __LINE__);                  \
        ESP_LOGW(ink_comp_name(comp), fmt, ##__VA_ARGS__);               \
    } while (0)
#define INK_LOGI(comp, fmt, ...) ESP_LOGI(ink_comp_name(comp), fmt, ##__VA_ARGS__)
#define INK_LOGD(comp, fmt, ...) ESP_LOGD(ink_comp_name(comp), fmt, ##__VA_ARGS__)
