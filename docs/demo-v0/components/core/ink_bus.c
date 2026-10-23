#include "ink_bus.h"
#include "ink_log.h"

ESP_EVENT_DEFINE_BASE(INK_EVT);

static esp_event_loop_handle_t s_loop;

size_t ink_bus_evt_size(int32_t id)
{
    switch (id) {
    case INK_EVT_BUTTON: return sizeof(ink_button_evt_t);
    case INK_EVT_DEVICE_DEGRADED: return sizeof(ink_comp_t);
    case INK_EVT_DEMO_CMD: return sizeof(uint8_t);
    default: return 0;
    }
}

esp_err_t ink_bus_init(void)
{
    esp_event_loop_args_t args = {
        .queue_size = 32,
        .task_name = "ink_bus",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY,
    };
    esp_err_t err = esp_event_loop_create(&args, &s_loop);
    INK_LOGI(INK_COMP_BUS, INK_LOG_EVT_INIT, "bus init: %s", esp_err_to_name(err));
    return err;
}

esp_err_t ink_bus_post(ink_evt_id_t id, const void *data, size_t size)
{
    return esp_event_post_to(s_loop, INK_EVT, id, data, size, pdMS_TO_TICKS(50));
}

esp_err_t ink_bus_post_from_isr(ink_evt_id_t id, const void *data, size_t size)
{
    return esp_event_isr_post_to(s_loop, INK_EVT, id, data, size, NULL);
}

esp_err_t ink_bus_subscribe(int32_t id, esp_event_handler_t handler, void *arg)
{
    return esp_event_handler_instance_register_with(s_loop, INK_EVT, id, handler, arg, NULL);
}
