#pragma once

#include <stddef.h>
#include "esp_err.h"
#include "ink_bus.h"
#include "ink_canvas.h"
#include "svc_render.h"

/*
 * Interface app (docs/adr/0004-vong-doi-app-task.md).
 * App KHÔNG phải task: mọi app chạy trong một task duy nhất (app runner), tại một thời điểm chỉ một app foreground.
 * Mọi callback chạy trong app runner nên app không cần khoá; callback phải trả về nhanh (Task Watchdog theo dõi).
 */
typedef struct ink_app {
    const char *name;
    void (*on_start)(void);                  /* cấp phát bằng app_alloc, đăng ký thứ cần */
    void (*on_event)(ink_intent_t intent);   /* chỉ nhận intent ngữ nghĩa, không bao giờ nhận số nút */
    void (*on_draw)(ink_canvas_t *canvas);   /* vẽ CẢ màn vào canvas (đã xoá trắng sẵn) */
    void (*on_stop)(void);                   /* giải phóng thứ đã cấp ở on_start */
} ink_app_t;

esp_err_t app_manager_start(const ink_app_t *first);
/* Chuyển app (bất đồng bộ): on_stop app cũ → dọn rò rỉ → on_start app mới → vẽ. */
esp_err_t app_manager_switch(const ink_app_t *app);
/* Gọi từ app khi nội dung đổi; app runner gọi on_draw rồi gửi render service. */
void app_request_redraw(ink_render_hint_t hint);
const ink_app_t *app_manager_current(void);

/*
 * Bộ nhớ của app: dùng thay malloc/free. App manager ghi lại mọi khối của app đang chạy;
 * khối nào còn sau on_stop sẽ bị giải phóng và ghi log là rò rỉ. Chỉ gọi từ callback của app.
 */
void *app_alloc(size_t size);
void *app_calloc(size_t n, size_t size);
void app_free(void *ptr);
size_t app_live_allocs(void);
