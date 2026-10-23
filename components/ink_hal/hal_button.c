#include "esp_attr.h"
#include "esp_timer.h"
#include "ink_bus.h"
#include "ink_hal_button.h"

static const ink_button_ops_t *s_ops;
static void *s_ctx;

esp_err_t ink_button_attach(const ink_button_ops_t *ops, void *ctx)
{
    if (!ops || !ops->count) {
        return ESP_ERR_INVALID_ARG;
    }
    s_ctx = ctx;
    s_ops = ops;
    return ESP_OK;
}

void ink_button_report(uint8_t index, bool pressed)
{
    const ink_button_raw_t evt = {
        .button = index,
        .pressed = pressed,
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000),
    };
    ink_bus_post(INK_EVT_BUTTON_RAW, &evt, sizeof(evt));
}

void IRAM_ATTR ink_button_report_from_isr(uint8_t index, bool pressed)
{
    const ink_button_raw_t evt = {
        .button = index,
        .pressed = pressed,
        .time_ms = (uint32_t)(esp_timer_get_time() / 1000),
    };
    ink_bus_post_from_isr(INK_EVT_BUTTON_RAW, &evt, sizeof(evt));
}

bool ink_button_present(void)
{
    return s_ops != NULL;
}

uint8_t ink_button_count(void)
{
    return s_ops ? s_ops->count(s_ctx) : 0;
}

bool ink_button_is_pressed(uint8_t index)
{
    return (s_ops && s_ops->is_pressed) ? s_ops->is_pressed(s_ctx, index) : false;
}
