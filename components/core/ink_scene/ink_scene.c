/* Design: docs/design/app-framework.md */
#include "ink_scene.h"

#include <stdbool.h>

/* ---- Leaf functions ---- */
/* Bodies marked LEAF are implemented later from the spec; the public functions in "Step lists" only call them. */

/* Set one entry to `scene_id` and a copy of `state`; a NULL `state` zeroes the state bytes. */
static void entry_write(ink_scene_entry_t *entry, uint8_t scene_id, const uint8_t *state) {
    /* LEAF: set entry->scene_id; copy INK_SCENE_STATE_BYTES bytes from state, or zero them when state is NULL. */
    (void)entry;
    (void)scene_id;
    (void)state;
}

/* True when the snapshot depth is 1..INK_SCENE_STACK_MAX and every scene id of the used entries is below scene_count. */
static bool snapshot_valid(const ink_scene_snapshot_t *snapshot, uint8_t scene_count) {
    /* LEAF: check depth range and each used entry's scene_id against scene_count. */
    (void)snapshot;
    (void)scene_count;
    return false;
}

/* ---- Public leaves ---- */

void ink_scene_init(ink_scene_stack_t *stack, uint8_t root_id) {
    /* LEAF: depth = 1, entries[0] = root_id with zeroed state; do nothing if stack is NULL. */
    (void)stack;
    (void)root_id;
}

ink_scene_error_t ink_scene_pop(ink_scene_stack_t *stack) {
    /* LEAF: ARGUMENT if stack is NULL; EMPTY if depth <= 1; otherwise depth-- and OK. */
    (void)stack;
    return INK_SCENE_ERROR_ARGUMENT;
}

void ink_scene_reset(ink_scene_stack_t *stack) {
    /* LEAF: depth = 1; the root entry and its state stay; do nothing if stack is NULL. */
    (void)stack;
}

ink_scene_entry_t *ink_scene_top(ink_scene_stack_t *stack) {
    /* LEAF: &entries[depth - 1]; NULL if stack is NULL or depth is 0. */
    (void)stack;
    return nullptr;
}

uint8_t ink_scene_depth(const ink_scene_stack_t *stack) {
    /* LEAF: depth; 0 if stack is NULL. */
    (void)stack;
    return 0;
}

const ink_scene_entry_t *ink_scene_entry_at(const ink_scene_stack_t *stack, uint8_t index) {
    /* LEAF: &entries[index]; NULL if stack is NULL or index >= depth. */
    (void)stack;
    (void)index;
    return nullptr;
}

void ink_scene_save(const ink_scene_stack_t *stack, ink_scene_snapshot_t *out) {
    /* LEAF: copy depth and the used entries into out; do nothing if a pointer is NULL. */
    (void)stack;
    (void)out;
}

/* ---- Step lists ---- */

ink_scene_error_t ink_scene_push(ink_scene_stack_t *stack, uint8_t scene_id, const uint8_t *state) {
    if (stack == nullptr) { return INK_SCENE_ERROR_ARGUMENT; }
    if (stack->priv.depth >= INK_SCENE_STACK_MAX) { return INK_SCENE_ERROR_FULL; }
    entry_write(&stack->priv.entries[stack->priv.depth], scene_id, state);
    stack->priv.depth++;
    return INK_SCENE_OK;
}

ink_scene_error_t ink_scene_replace(ink_scene_stack_t *stack, uint8_t scene_id, const uint8_t *state) {
    ink_scene_entry_t *top = ink_scene_top(stack);
    if (top == nullptr) { return INK_SCENE_ERROR_ARGUMENT; }
    entry_write(top, scene_id, state);
    return INK_SCENE_OK;
}

ink_scene_error_t ink_scene_restore(ink_scene_stack_t *stack, const ink_scene_snapshot_t *in, uint8_t scene_count) {
    if (stack == nullptr || in == nullptr) { return INK_SCENE_ERROR_ARGUMENT; }
    if (!snapshot_valid(in, scene_count)) { return INK_SCENE_ERROR_SNAPSHOT; }
    stack->priv.depth = in->depth;
    for (uint8_t i = 0; i < in->depth; i++) { stack->priv.entries[i] = in->entries[i]; }
    return INK_SCENE_OK;
}
