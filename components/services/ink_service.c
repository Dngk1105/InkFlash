#include <stdlib.h>
#include "sdkconfig.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_service.h"
#include "ink_health.h"
#include "ink_log.h"

#define QUEUE_LEN 8

typedef struct {
    const ink_service_def_t *def;
    QueueHandle_t queue;
} svc_ctx_t;

static void bus_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    svc_ctx_t *ctx = arg;
    ink_svc_msg_t msg = { .id = id };
    size_t size = ink_bus_evt_size(id);
    if (data && size) {
        memcpy(msg.data, data, size < INK_SVC_MSG_DATA_MAX ? size : INK_SVC_MSG_DATA_MAX);
    }
    if (xQueueSend(ctx->queue, &msg, 0) != pdTRUE) {
        INK_LOGW(ctx->def->comp, INK_LOG_EVT_STATE, "queue full, drop evt %ld", (long)id);
    }
}

static void service_task(void *arg)
{
    svc_ctx_t *ctx = arg;
    if (ctx->def->on_start) {
        ctx->def->on_start();
    }
    ink_health_set(ctx->def->comp, INK_HEALTH_OK);
    ink_svc_msg_t msg;
    TickType_t wait = ctx->def->tick_ms ? pdMS_TO_TICKS(ctx->def->tick_ms) : portMAX_DELAY;
    for (;;) {
        if (xQueueReceive(ctx->queue, &msg, wait) == pdTRUE) {
            if (ctx->def->on_event) {
                ctx->def->on_event(&msg);
            }
        } else if (ctx->def->on_tick) {
            ctx->def->on_tick();
        }
    }
}

esp_err_t ink_service_start(const ink_service_def_t *def)
{
    svc_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        return ESP_ERR_NO_MEM;
    }
    ctx->def = def;
    ctx->queue = xQueueCreate(QUEUE_LEN, sizeof(ink_svc_msg_t));
    if (!ctx->queue) {
        free(ctx);
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = ink_bus_subscribe(ESP_EVENT_ANY_ID, bus_handler, ctx);
    if (err != ESP_OK) {
        return err;
    }
    if (xTaskCreate(service_task, def->name, def->stack_size, ctx, def->priority, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    INK_LOGI(def->comp, INK_LOG_EVT_INIT, "started");
    return ESP_OK;
}

esp_err_t ink_services_start(void)
{
    const ink_service_def_t *defs[] = {
        &svc_storage_def, /* trước GUI: GUI cần dữ liệu thẻ */
        &svc_power_def,
        &svc_gui_def,
        &svc_audio_def,
        &svc_ai_def,
        &svc_gesture_def,
#if CONFIG_INK_DEMO
        &svc_demo_def,
#endif
    };
    for (size_t i = 0; i < sizeof(defs) / sizeof(defs[0]); i++) {
        esp_err_t err = ink_service_start(defs[i]);
        if (err != ESP_OK) {
            INK_LOGE(defs[i]->comp, INK_LOG_EVT_SELF_TEST_FAIL, "start fail: %s", esp_err_to_name(err));
            ink_health_set(defs[i]->comp, INK_HEALTH_DEGRADED);
        }
    }
    return ESP_OK;
}
