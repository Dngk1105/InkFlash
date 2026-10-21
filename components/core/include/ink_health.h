#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "ink_comp.h"

typedef enum {
    INK_HEALTH_UNKNOWN = 0,
    INK_HEALTH_OK,
    INK_HEALTH_DEGRADED,
} ink_health_t;

/* Ghi nhận kết quả một thao tác. Lỗi liên tiếp vượt ngưỡng → DEGRADED + phát INK_EVT_DEVICE_DEGRADED. */
void ink_health_report(ink_comp_t comp, bool ok);
void ink_health_set(ink_comp_t comp, ink_health_t state);
ink_health_t ink_health_get(ink_comp_t comp);
uint32_t ink_health_fail_count(ink_comp_t comp);
const char *ink_health_str(ink_health_t state);
