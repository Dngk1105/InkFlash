#include "ink_bus.h"
#include "freertos/FreeRTOS.h"
#include "ink_log.h"

ESP_EVENT_DEFINE_BASE(INK_EVT);

#define BUS_QUEUE_SIZE 32
#define BUS_POST_TIMEOUT_MS 20

static esp_event_loop_handle_t s_loop;
static volatile uint32_t s_drops;

size_t ink_bus_evt_size(int32_t id)
{
    switch (id) {
    case INK_EVT_BUTTON_RAW: return sizeof(ink_button_raw_t);
    case INK_EVT_INTENT: return sizeof(ink_intent_evt_t);
    case INK_EVT_POWER_STATE: return sizeof(ink_power_state_evt_t);
    case INK_EVT_RENDER_DONE: return sizeof(ink_render_done_t);
    case INK_EVT_DEVICE_DEGRADED: return sizeof(ink_comp_t);
    default: return 0;
    }
}

esp_err_t ink_bus_init(void)
{
    const esp_event_loop_args_t args = {
        .queue_size = BUS_QUEUE_SIZE,
        .task_name = "ink_bus",
        .task_priority = 6,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY,
    };
    return esp_event_loop_create(&args, &s_loop);
}

static esp_err_t count_drop(esp_err_t err, ink_evt_id_t id)
{
    if (err != ESP_OK) {
        s_drops++;
        INK_LOGW(INK_COMP_BUS, "post evt %d that bai: %s", (int)id, esp_err_to_name(err));
    }
    return err;
}

esp_err_t ink_bus_post(ink_evt_id_t id, const void *data, size_t size)
{
    return count_drop(esp_event_post_to(s_loop, INK_EVT, id, data, size, pdMS_TO_TICKS(BUS_POST_TIMEOUT_MS)), id);
}

esp_err_t ink_bus_post_from_isr(ink_evt_id_t id, const void *data, size_t size)
{
    esp_err_t err = esp_event_isr_post_to(s_loop, INK_EVT, id, data, size, NULL);
    if (err != ESP_OK) {
        s_drops++; /* không log trong ISR */
    }
    return err;
}

esp_err_t ink_bus_subscribe(int32_t id, esp_event_handler_t handler, void *arg,
                            esp_event_handler_instance_t *out_instance)
{
    return esp_event_handler_instance_register_with(s_loop, INK_EVT, id, handler, arg, out_instance);
}

esp_err_t ink_bus_unsubscribe(int32_t id, esp_event_handler_instance_t instance)
{
    return esp_event_handler_instance_unregister_with(s_loop, INK_EVT, id, instance);
}

uint32_t ink_bus_drop_count(void)
{
    return s_drops;
}
