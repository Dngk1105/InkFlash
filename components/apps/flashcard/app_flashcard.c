#include "ink_apps.h"
#include "ink_log.h"
#include "ink_scene.h"

/*
 * App Flashcard — KHUNG. Luồng: DeckSelect → Question → Answer → (chấm điểm) → Question ... → Summary.
 * Chưa có font (GĐ1) và dữ liệu (GĐ2): mỗi scene vẽ số ô vuông bằng số thứ tự scene để nhận biết.
 * GĐ3: nội dung thẻ thật, chấm điểm ghi ReviewLog, FSRS chọn thẻ kế tiếp.
 */
enum { SC_DECK_SELECT, SC_QUESTION, SC_ANSWER, SC_SUMMARY, SC_COUNT };

static ink_scene_mgr_t s_scenes;
static uint32_t *s_reviewed; /* ví dụ cấp phát qua app_alloc */

static void draw_marker(ink_canvas_t *c, int n)
{
    ink_canvas_rect(c, 4, 4, c->w - 8, c->h - 8, INK_BLACK);
    for (int i = 0; i < n; i++) {
        ink_canvas_fill_rect(c, 20 + i * 40, 20, 30, 30, INK_BLACK);
    }
}

static bool deck_event(ink_intent_t intent)
{
    if (intent == INK_INTENT_SELECT) {
        ink_scene_goto(&s_scenes, SC_QUESTION);
        return true;
    }
    return false;
}

static bool question_event(ink_intent_t intent)
{
    if (intent == INK_INTENT_SELECT || intent == INK_INTENT_FLIP) {
        ink_scene_goto(&s_scenes, SC_ANSWER);
        return true;
    }
    return false;
}

static bool answer_event(ink_intent_t intent)
{
    if ((intent == INK_INTENT_NEXT || intent == INK_INTENT_GRADE_GOOD) && s_reviewed) {
        (*s_reviewed)++; /* GĐ3: ghi ReviewLog, FSRS chọn thẻ kế */
        ink_scene_goto(&s_scenes, *s_reviewed >= 5 ? SC_SUMMARY : SC_QUESTION);
        return true;
    }
    return false;
}

static bool summary_event(ink_intent_t intent)
{
    if (intent == INK_INTENT_SELECT && s_reviewed) {
        *s_reviewed = 0;
        ink_scene_goto(&s_scenes, SC_DECK_SELECT);
        return true;
    }
    return false;
}

static void deck_draw(ink_canvas_t *c) { draw_marker(c, 1); }
static void question_draw(ink_canvas_t *c) { draw_marker(c, 2); }
static void answer_draw(ink_canvas_t *c) { draw_marker(c, 3); }
static void summary_draw(ink_canvas_t *c) { draw_marker(c, 4); }

static const ink_scene_t s_deck = { "deck_select", NULL, deck_event, deck_draw, NULL };
static const ink_scene_t s_question = { "question", NULL, question_event, question_draw, NULL };
static const ink_scene_t s_answer = { "answer", NULL, answer_event, answer_draw, NULL };
static const ink_scene_t s_summary = { "summary", NULL, summary_event, summary_draw, NULL };
static const ink_scene_t *const s_list[SC_COUNT] = { &s_deck, &s_question, &s_answer, &s_summary };

static void on_start(void)
{
    s_reviewed = app_calloc(1, sizeof(*s_reviewed));
    ink_scene_init(&s_scenes, s_list, SC_COUNT, SC_DECK_SELECT);
}

static void on_event(ink_intent_t intent)
{
    if (intent == INK_INTENT_MENU) {
        app_manager_switch(&app_settings);
        return;
    }
    const uint8_t before = s_scenes.current;
    ink_scene_event(&s_scenes, intent);
    if (s_scenes.current != before) {
        INK_LOGI(INK_COMP_APP, "flashcard: %s", ink_scene_current(&s_scenes)->name);
        app_request_redraw(INK_RENDER_FAST);
    }
}

static void on_draw(ink_canvas_t *canvas)
{
    ink_scene_draw(&s_scenes, canvas);
}

static void on_stop(void)
{
    app_free(s_reviewed);
    s_reviewed = NULL;
}

const ink_app_t app_flashcard = {
    .name = "flashcard",
    .on_start = on_start,
    .on_event = on_event,
    .on_draw = on_draw,
    .on_stop = on_stop,
};
