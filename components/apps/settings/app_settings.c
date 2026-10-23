#include "ink_apps.h"

/*
 * App Settings — KHUNG. GĐ3: chọn deck mặc định, âm lượng, thời gian tự ngủ, thông tin máy.
 * Cũng là app mở thẳng khi boot vào safe mode.
 */
static void on_event(ink_intent_t intent)
{
    if (intent == INK_INTENT_BACK || intent == INK_INTENT_MENU) {
        app_manager_switch(&app_flashcard);
    }
}

static void on_draw(ink_canvas_t *c)
{
    /* Đánh dấu tạm: khung kép (chưa có font) */
    ink_canvas_rect(c, 4, 4, c->w - 8, c->h - 8, INK_BLACK);
    ink_canvas_rect(c, 12, 12, c->w - 24, c->h - 24, INK_BLACK);
}

const ink_app_t app_settings = {
    .name = "settings",
    .on_event = on_event,
    .on_draw = on_draw,
};
