#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "ink_bus.h"
#include "ink_canvas.h"

/*
 * Scene: máy trạng thái bên trong một app (học từ Flipper Zero SceneManager).
 * Ví dụ Flashcard: DeckSelect → Question → Answer → Summary.
 */
typedef struct {
    const char *name;
    void (*on_enter)(void);
    bool (*on_event)(ink_intent_t intent); /* true = đã xử lý */
    void (*on_draw)(ink_canvas_t *canvas);
    void (*on_exit)(void);
} ink_scene_t;

typedef struct {
    const ink_scene_t *const *scenes;
    uint8_t count;
    uint8_t current;
} ink_scene_mgr_t;

void ink_scene_init(ink_scene_mgr_t *m, const ink_scene_t *const *scenes, uint8_t count, uint8_t first);
void ink_scene_goto(ink_scene_mgr_t *m, uint8_t index); /* on_exit cảnh cũ → on_enter cảnh mới */
bool ink_scene_event(ink_scene_mgr_t *m, ink_intent_t intent);
void ink_scene_draw(ink_scene_mgr_t *m, ink_canvas_t *canvas);
const ink_scene_t *ink_scene_current(const ink_scene_mgr_t *m);
