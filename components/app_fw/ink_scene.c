#include "ink_scene.h"

void ink_scene_init(ink_scene_mgr_t *m, const ink_scene_t *const *scenes, uint8_t count, uint8_t first)
{
    m->scenes = scenes;
    m->count = count;
    m->current = first < count ? first : 0;
    if (m->scenes[m->current]->on_enter) {
        m->scenes[m->current]->on_enter();
    }
}

void ink_scene_goto(ink_scene_mgr_t *m, uint8_t index)
{
    if (index >= m->count || index == m->current) {
        return;
    }
    if (m->scenes[m->current]->on_exit) {
        m->scenes[m->current]->on_exit();
    }
    m->current = index;
    if (m->scenes[index]->on_enter) {
        m->scenes[index]->on_enter();
    }
}

bool ink_scene_event(ink_scene_mgr_t *m, ink_intent_t intent)
{
    const ink_scene_t *s = m->scenes[m->current];
    return s->on_event ? s->on_event(intent) : false;
}

void ink_scene_draw(ink_scene_mgr_t *m, ink_canvas_t *canvas)
{
    const ink_scene_t *s = m->scenes[m->current];
    if (s->on_draw) {
        s->on_draw(canvas);
    }
}

const ink_scene_t *ink_scene_current(const ink_scene_mgr_t *m)
{
    return m->scenes[m->current];
}
