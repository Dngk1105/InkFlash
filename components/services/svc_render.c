#include <string.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ink_bus.h"
#include "ink_hal_display.h"
#include "ink_log.h"
#include "ink_service.h"
#include "svc_internal.h"
#include "svc_render.h"

/*
 * Hai buffer cấp một lần lúc khởi động (đường nóng, docs/adr/0003):
 *  - s_pending: canvas mới nhất app gửi tới (ghi dưới khoá).
 *  - s_shown:   ảnh đang/vừa hiển thị, nằm trong RAM nội có DMA để driver đẩy thẳng ra SPI.
 */
enum { CMD_RENDER = 1 };

static ink_service_t *s_svc;
static ink_display_info_t s_info;
static uint8_t *s_pending;
static uint8_t *s_shown;
static uint32_t s_bytes;
static SemaphoreHandle_t s_lock;
static bool s_has_pending;
static ink_render_hint_t s_pending_hint;
static uint32_t s_pending_id;
static uint32_t s_next_id = 1;

bool render_available(void)
{
    return s_shown != NULL;
}

esp_err_t render_get_size(uint16_t *w, uint16_t *h)
{
    if (!render_available()) {
        return ESP_ERR_NOT_FOUND;
    }
    *w = s_info.width;
    *h = s_info.height;
    return ESP_OK;
}

esp_err_t render_submit(const ink_canvas_t *canvas, ink_render_hint_t hint, uint32_t *out_request_id)
{
    if (!render_available()) {
        return ESP_ERR_NOT_FOUND;
    }
    if (canvas->w != s_info.width || canvas->h != s_info.height) {
        return ESP_ERR_INVALID_SIZE;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(s_pending, canvas->buf, s_bytes);
    const bool was_pending = s_has_pending;
    s_has_pending = true;
    s_pending_hint = hint;
    s_pending_id = s_next_id++;
    if (out_request_id) {
        *out_request_id = s_pending_id;
    }
    xSemaphoreGive(s_lock);
    /* Đã có lệnh chờ trong queue thì không gửi thêm: lần vẽ đó sẽ lấy canvas mới nhất (coalescing). */
    return was_pending ? ESP_OK : ink_service_send(s_svc, INK_SVC_MSG_CMD, CMD_RENDER, NULL, 0);
}

static ink_refresh_t choose_mode(ink_render_hint_t hint)
{
    /* GĐ1: chính sách đầy đủ (đếm số lần partial, kích thước vùng đổi, deep clean định kỳ). */
    switch (hint) {
    case INK_RENDER_CLEAN: return INK_REFRESH_FULL;
    case INK_RENDER_FAST: return INK_REFRESH_PARTIAL;
    default: return INK_REFRESH_PARTIAL;
    }
}

static void do_render(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!s_has_pending) {
        xSemaphoreGive(s_lock);
        return;
    }
    memcpy(s_shown, s_pending, s_bytes);
    s_has_pending = false;
    const ink_render_hint_t hint = s_pending_hint;
    const uint32_t id = s_pending_id;
    xSemaphoreGive(s_lock);

    const int64_t start = esp_timer_get_time();
    const esp_err_t err = ink_display_draw(s_shown, NULL, choose_mode(hint));
    const ink_render_done_t done = {
        .request_id = id,
        .duration_ms = (uint32_t)((esp_timer_get_time() - start) / 1000),
        .result = err,
    };
    ink_bus_post(INK_EVT_RENDER_DONE, &done, sizeof(done));
}

static void on_start(void)
{
    if (ink_display_get_info(&s_info) != ESP_OK) {
        INK_LOGW(INK_COMP_SVC_RENDER, "board khong co man hinh");
        return;
    }
    s_bytes = (uint32_t)((s_info.width + 7) / 8) * s_info.height;
    s_shown = heap_caps_malloc(s_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_pending = heap_caps_malloc(s_bytes, MALLOC_CAP_SPIRAM);
    if (!s_shown || !s_pending) {
        INK_LOGE(INK_COMP_SVC_RENDER, "thieu bo nho cho framebuffer (%lu B)", (unsigned long)s_bytes);
        heap_caps_free(s_shown);
        heap_caps_free(s_pending);
        s_shown = s_pending = NULL;
        return;
    }
    INK_LOGI(INK_COMP_SVC_RENDER, "man %ux%u", s_info.width, s_info.height);
}

static void on_msg(const ink_svc_msg_t *msg)
{
    if (msg->type == INK_SVC_MSG_CMD && msg->id == CMD_RENDER) {
        do_render();
    } else if (msg->type == INK_SVC_MSG_SUSPEND) {
        ink_display_sleep();
    }
}

static const ink_service_def_t s_def = {
    .name = "svc_render",
    .comp = INK_COMP_SVC_RENDER,
    .stack_size = 4096,
    .priority = 5,
    .on_start = on_start,
    .on_msg = on_msg,
};

esp_err_t svc_render_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) {
        return ESP_ERR_NO_MEM;
    }
    return ink_service_start(&s_def, &s_svc);
}
