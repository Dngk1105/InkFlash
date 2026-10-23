#include "ink_health.h"
#include "ink_bus.h"
#include "ink_log.h"

#define DEGRADED_AFTER_FAILS 3

static ink_health_t s_state[INK_COMP_COUNT];
static uint32_t s_streak[INK_COMP_COUNT];
static uint32_t s_total[INK_COMP_COUNT];

void ink_health_set(ink_comp_t comp, ink_health_t state)
{
    if (comp >= INK_COMP_COUNT || s_state[comp] == state) {
        return;
    }
    s_state[comp] = state;
    if (state == INK_HEALTH_DEGRADED) {
        INK_LOGE(comp, "DEGRADED");
        ink_bus_post(INK_EVT_DEVICE_DEGRADED, &comp, sizeof(comp));
    }
}

void ink_health_report(ink_comp_t comp, bool ok)
{
    if (comp >= INK_COMP_COUNT) {
        return;
    }
    if (ok) {
        s_streak[comp] = 0;
        ink_health_set(comp, INK_HEALTH_OK);
        return;
    }
    s_total[comp]++;
    if (++s_streak[comp] >= DEGRADED_AFTER_FAILS) {
        ink_health_set(comp, INK_HEALTH_DEGRADED);
    }
}

ink_health_t ink_health_get(ink_comp_t comp)
{
    return comp < INK_COMP_COUNT ? s_state[comp] : INK_HEALTH_UNKNOWN;
}

uint32_t ink_health_fail_count(ink_comp_t comp)
{
    return comp < INK_COMP_COUNT ? s_total[comp] : 0;
}

const char *ink_health_str(ink_health_t state)
{
    switch (state) {
    case INK_HEALTH_OK: return "OK";
    case INK_HEALTH_DEGRADED: return "DEGRADED";
    case INK_HEALTH_ABSENT: return "ABSENT";
    default: return "UNKNOWN";
    }
}
