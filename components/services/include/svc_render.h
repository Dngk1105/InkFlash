#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "ink_canvas.h"

/*
 * Render service: nhận canvas từ app và đẩy ra màn trong task riêng. App không bao giờ phải chờ màn vẽ xong.
 *  - render_submit() chép canvas vào buffer của service rồi trả về ngay.
 *  - Coalescing: đang vẽ mà có nhiều canvas mới tới thì chỉ vẽ canvas mới nhất.
 *  - Xong mỗi lần vẽ, service phát INK_EVT_RENDER_DONE (request_id, thời gian, kết quả).
 * GĐ1: dirty rect (chỉ làm mới vùng đổi), chính sách full/partial/clean tự động, prefetch.
 */
typedef enum {
    INK_RENDER_AUTO,  /* service tự chọn partial/full theo chính sách */
    INK_RENDER_FAST,  /* ưu tiên nhanh (partial) */
    INK_RENDER_CLEAN, /* ưu tiên sạch (full hoặc deep clean) */
} ink_render_hint_t;

bool render_available(void);
/* Kích thước màn logic; dùng để app tạo canvas đúng cỡ. ESP_ERR_NOT_FOUND nếu không có màn. */
esp_err_t render_get_size(uint16_t *w, uint16_t *h);
esp_err_t render_submit(const ink_canvas_t *canvas, ink_render_hint_t hint, uint32_t *out_request_id);
