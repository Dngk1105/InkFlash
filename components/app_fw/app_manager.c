#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_app.h"
#include "ink_log.h"

#define RUNNER_STACK 6144
#define RUNNER_PRIO 4
#define LOOP_TIMEOUT_MS 1000 /* nhỏ hơn hạn Task Watchdog để còn kịp reset bộ đếm */

typedef enum { MSG_INTENT, MSG_SWITCH } msg_type_t;

typedef struct {
    msg_type_t type;
    union {
        ink_intent_t intent;
        const ink_app_t *app;
    };
} runner_msg_t;

/* Mỗi khối app_alloc có header để nối vào danh sách theo dõi. */
typedef struct alloc_hdr {
    struct alloc_hdr *next, *prev;
    size_t size;
} alloc_hdr_t;

static QueueHandle_t s_queue;
static const ink_app_t *s_current;
static alloc_hdr_t *s_allocs;
static size_t s_alloc_count;
static ink_canvas_t s_canvas;
static bool s_redraw;
static ink_render_hint_t s_redraw_hint;

/* ---- Bộ nhớ theo app ---- */

void *app_alloc(size_t size)
{
    alloc_hdr_t *h = heap_caps_malloc(sizeof(*h) + size, MALLOC_CAP_DEFAULT);
    if (!h) {
        INK_LOGW(INK_COMP_APP_MANAGER, "app_alloc %u B that bai", (unsigned)size);
        return NULL;
    }
    h->size = size;
    h->prev = NULL;
    h->next = s_allocs;
    if (s_allocs) {
        s_allocs->prev = h;
    }
    s_allocs = h;
    s_alloc_count++;
    return h + 1;
}

void *app_calloc(size_t n, size_t size)
{
    void *p = app_alloc(n * size);
    if (p) {
        memset(p, 0, n * size);
    }
    return p;
}

void app_free(void *ptr)
{
    if (!ptr) {
        return;
    }
    alloc_hdr_t *h = (alloc_hdr_t *)ptr - 1;
    if (h->prev) {
        h->prev->next = h->next;
    } else {
        s_allocs = h->next;
    }
    if (h->next) {
        h->next->prev = h->prev;
    }
    s_alloc_count--;
    heap_caps_free(h);
}

size_t app_live_allocs(void)
{
    return s_alloc_count;
}

static void reclaim_leaks(const ink_app_t *app)
{
    size_t count = 0, bytes = 0;
    while (s_allocs) {
        bytes += s_allocs->size;
        count++;
        app_free(s_allocs + 1);
    }
    if (count) {
        INK_LOGW(INK_COMP_APP_MANAGER, "app %s ro ri %u khoi (%u B): da thu hoi", app->name, (unsigned)count,
                 (unsigned)bytes);
    }
}

/* ---- App runner ---- */

static void draw_current(void)
{
    s_redraw = false;
    if (!s_current || !s_current->on_draw || !s_canvas.buf) {
        return;
    }
    ink_canvas_clear(&s_canvas, INK_WHITE);
    s_current->on_draw(&s_canvas);
    esp_err_t err = render_submit(&s_canvas, s_redraw_hint, NULL);
    if (err != ESP_OK) {
        INK_LOGW(INK_COMP_APP_MANAGER, "render_submit: %s", esp_err_to_name(err));
    }
}

static void switch_to(const ink_app_t *app)
{
    if (s_current) {
        INK_LOGI(INK_COMP_APP_MANAGER, "dung app %s", s_current->name);
        if (s_current->on_stop) {
            s_current->on_stop();
        }
        reclaim_leaks(s_current);
    }
    s_current = app;
    INK_LOGI(INK_COMP_APP_MANAGER, "mo app %s", app->name);
    if (app->on_start) {
        app->on_start();
    }
    app_request_redraw(INK_RENDER_CLEAN);
}

static void runner_task(void *arg)
{
    esp_task_wdt_add(NULL); /* app treo trong callback → watchdog reset chip → boot đếm crash → safe mode (GĐ3) */
    switch_to((const ink_app_t *)arg);
    runner_msg_t msg;
    for (;;) {
        esp_task_wdt_reset();
        if (xQueueReceive(s_queue, &msg, pdMS_TO_TICKS(LOOP_TIMEOUT_MS)) == pdTRUE) {
            switch (msg.type) {
            case MSG_INTENT:
                if (s_current && s_current->on_event) {
                    s_current->on_event(msg.intent);
                }
                break;
            case MSG_SWITCH:
                switch_to(msg.app);
                break;
            }
        }
        if (s_redraw) {
            draw_current();
        }
    }
}

static void intent_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const runner_msg_t msg = { .type = MSG_INTENT, .intent = ((const ink_intent_evt_t *)data)->intent };
    if (xQueueSend(s_queue, &msg, 0) != pdTRUE) {
        INK_LOGW(INK_COMP_APP_MANAGER, "queue day, bo intent %d", (int)msg.intent);
    }
}

void app_request_redraw(ink_render_hint_t hint)
{
    /* Gọi từ app (trong runner): chỉ đặt cờ, vẽ một lần cuối vòng lặp dù app gọi nhiều lần.
       Yêu cầu CLEAN đang chờ không bị hạ xuống; còn lại lấy hint mới nhất. */
    if (!s_redraw || s_redraw_hint != INK_RENDER_CLEAN) {
        s_redraw_hint = hint;
    }
    s_redraw = true;
}

esp_err_t app_manager_switch(const ink_app_t *app)
{
    const runner_msg_t msg = { .type = MSG_SWITCH, .app = app };
    return xQueueSend(s_queue, &msg, pdMS_TO_TICKS(50)) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

const ink_app_t *app_manager_current(void)
{
    return s_current;
}

esp_err_t app_manager_start(const ink_app_t *first)
{
    s_queue = xQueueCreate(8, sizeof(runner_msg_t));
    if (!s_queue) {
        return ESP_ERR_NO_MEM;
    }
    uint16_t w, h;
    if (render_get_size(&w, &h) == ESP_OK) {
        uint8_t *buf = heap_caps_malloc(ink_canvas_bytes(w, h), MALLOC_CAP_SPIRAM);
        if (!buf) {
            return ESP_ERR_NO_MEM;
        }
        ink_canvas_init(&s_canvas, buf, w, h);
    } else {
        INK_LOGW(INK_COMP_APP_MANAGER, "khong co man: app chay khong ve");
    }
    esp_err_t err = ink_bus_subscribe(INK_EVT_INTENT, intent_handler, NULL, NULL);
    if (err != ESP_OK) {
        return err;
    }
    return xTaskCreate(runner_task, "app_runner", RUNNER_STACK, (void *)first, RUNNER_PRIO, NULL) == pdPASS
               ? ESP_OK
               : ESP_ERR_NO_MEM;
}
