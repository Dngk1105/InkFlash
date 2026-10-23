#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_bus.h"
#include "ink_health.h"
#include "ink_log.h"
#include "ink_service.h"
#include "svc_internal.h"

#define MAX_SERVICES 8

struct ink_service {
    const ink_service_def_t *def;
    QueueHandle_t queue;
    TaskHandle_t task;
};

static ink_service_t *s_services[MAX_SERVICES];
static int s_count;

static void bus_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    ink_service_t *svc = arg;
    ink_svc_msg_t msg = { .type = INK_SVC_MSG_EVENT, .id = id };
    size_t size = ink_bus_evt_size(id);
    if (data && size) {
        memcpy(msg.data, data, size < INK_SVC_DATA_MAX ? size : INK_SVC_DATA_MAX);
    }
    if (xQueueSend(svc->queue, &msg, 0) != pdTRUE) {
        INK_LOGW(svc->def->comp, "queue day, bo su kien %ld", (long)id);
    }
}

static void service_task(void *arg)
{
    ink_service_t *svc = arg;
    const ink_service_def_t *def = svc->def;
    if (def->on_start) {
        def->on_start();
    }
    ink_health_set(def->comp, INK_HEALTH_OK);
    const TickType_t wait = def->tick_ms ? pdMS_TO_TICKS(def->tick_ms) : portMAX_DELAY;
    ink_svc_msg_t msg;
    for (;;) {
        if (xQueueReceive(svc->queue, &msg, wait) == pdTRUE) {
            if (def->on_msg) {
                def->on_msg(&msg);
            }
        } else if (def->on_tick) {
            def->on_tick();
        }
    }
}

esp_err_t ink_service_start(const ink_service_def_t *def, ink_service_t **out)
{
    if (s_count >= MAX_SERVICES) {
        return ESP_ERR_NO_MEM;
    }
    ink_service_t *svc = calloc(1, sizeof(*svc));
    if (!svc) {
        return ESP_ERR_NO_MEM;
    }
    svc->def = def;
    svc->queue = xQueueCreate(def->queue_len ? def->queue_len : 8, sizeof(ink_svc_msg_t));
    if (!svc->queue) {
        free(svc);
        return ESP_ERR_NO_MEM;
    }
    for (const int32_t *e = def->events; e && *e >= 0; e++) {
        esp_err_t err = ink_bus_subscribe(*e, bus_handler, svc, NULL);
        if (err != ESP_OK) {
            return err;
        }
    }
    if (xTaskCreate(service_task, def->name, def->stack_size, svc, def->priority, &svc->task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    s_services[s_count++] = svc;
    if (out) {
        *out = svc;
    }
    return ESP_OK;
}

esp_err_t ink_service_send(ink_service_t *svc, ink_svc_msg_type_t type, int32_t id, const void *data, size_t size)
{
    if (!svc) {
        return ESP_ERR_INVALID_STATE;
    }
    if (size > INK_SVC_DATA_MAX) {
        return ESP_ERR_INVALID_SIZE;
    }
    ink_svc_msg_t msg = { .type = type, .id = id };
    if (data && size) {
        memcpy(msg.data, data, size);
    }
    return xQueueSend(svc->queue, &msg, pdMS_TO_TICKS(20)) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

void ink_services_broadcast(ink_svc_msg_type_t type)
{
    for (int i = 0; i < s_count; i++) {
        ink_service_send(s_services[i], type, 0, NULL, 0);
    }
}

esp_err_t ink_services_start(void)
{
    /* Thứ tự: storage trước (app cần dữ liệu), render trước app manager, power sau cùng. */
    esp_err_t (*const starters[])(void) = {
        svc_storage_start, svc_render_start, svc_audio_start, svc_input_start, svc_transfer_start, svc_power_start,
    };
    for (size_t i = 0; i < sizeof(starters) / sizeof(starters[0]); i++) {
        esp_err_t err = starters[i]();
        if (err != ESP_OK) {
            INK_LOGE(INK_COMP_SYS, "khoi dong service %u loi: %s", (unsigned)i, esp_err_to_name(err));
            return err;
        }
    }
    return ESP_OK;
}
