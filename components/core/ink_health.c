#include "ink_health.h"
#include "ink_bus.h"
#include "ink_log.h"

#define DEGRADED_THRESHOLD 3

static ink_health_t s_state[INK_COMP_COUNT];
static uint32_t s_fail_streak[INK_COMP_COUNT];
static uint32_t s_fail_total[INK_COMP_COUNT];

void ink_health_set(ink_comp_t comp, ink_health_t state)
{
    if (comp >= INK_COMP_COUNT || s_state[comp] == state) {
        return;
    }
    s_state[comp] = state;
    if (state == INK_HEALTH_DEGRADED) {
        INK_LOGE(comp, INK_LOG_EVT_DEGRADED, "DEGRADED");
        ink_bus_post(INK_EVT_DEVICE_DEGRADED, &comp, sizeof(comp));
    }
}

void ink_health_report(ink_comp_t comp, bool ok)
{
    if (comp >= INK_COMP_COUNT) {
        return;
    }
    if (ok) {
        s_fail_streak[comp] = 0;
        ink_health_set(comp, INK_HEALTH_OK);
        return;
    }
    s_fail_total[comp]++;
    if (++s_fail_streak[comp] >= DEGRADED_THRESHOLD) {
        ink_health_set(comp, INK_HEALTH_DEGRADED);
    }
}

ink_health_t ink_health_get(ink_comp_t comp)
{
    return comp < INK_COMP_COUNT ? s_state[comp] : INK_HEALTH_UNKNOWN;
}

uint32_t ink_health_fail_count(ink_comp_t comp)
{
    return comp < INK_COMP_COUNT ? s_fail_total[comp] : 0;
}

const char *ink_health_str(ink_health_t state)
{
    switch (state) {
    case INK_HEALTH_OK: return "OK";
    case INK_HEALTH_DEGRADED: return "DEGRADED";
    default: return "UNKNOWN";
    }
}
