/* Design: docs/design/app-framework.md */
#include "ink_scene.h"

#include <stdbool.h>
#include <string.h>

/* ---- Leaf functions ---- */

/* Set one entry to `scene_id` and a copy of `state`; a NULL `state` zeroes the state bytes. */
static void entry_write(ink_scene_entry_t *entry, uint8_t scene_id, const uint8_t *state) {
    if (entry == nullptr) { return; }
    entry->scene_id = scene_id;
    if (state != nullptr) {
        memcpy(entry->state, state, INK_SCENE_STATE_BYTES);
    } else {
        memset(entry->state, 0, INK_SCENE_STATE_BYTES);
    }
}

/* True when the snapshot depth is 1..INK_SCENE_STACK_MAX and every scene id of the used entries is below scene_count. */
static bool snapshot_valid(const ink_scene_snapshot_t *snapshot, uint8_t scene_count) {
    if (snapshot == nullptr) { return false; }
    if (snapshot->depth < 1 || snapshot->depth > INK_SCENE_STACK_MAX) { return false; }
    for (uint8_t i = 0; i < snapshot->depth; i++) {
        if (snapshot->entries[i].scene_id >= scene_count) { return false; }
    }
    return true;
}

/* ---- Public leaves ---- */

void ink_scene_init(ink_scene_stack_t *stack, uint8_t root_id) {
    if (stack == nullptr) { return; }
    memset(stack, 0, sizeof(*stack)); /* unused entries are zero, so a saved snapshot has no stale bytes */
    stack->priv.depth = 1;
    entry_write(&stack->priv.entries[0], root_id, nullptr);
}

ink_scene_error_t ink_scene_pop(ink_scene_stack_t *stack) {
    if (stack == nullptr) { return INK_SCENE_ERROR_ARGUMENT; }
    if (stack->priv.depth <= 1) { return INK_SCENE_ERROR_EMPTY; }
    stack->priv.depth--;
    return INK_SCENE_OK;
}

void ink_scene_reset(ink_scene_stack_t *stack) {
    if (stack == nullptr) { return; }
    stack->priv.depth = 1;
}

ink_scene_entry_t *ink_scene_top(ink_scene_stack_t *stack) {
    if (stack == nullptr || stack->priv.depth == 0) { return nullptr; }
    return &stack->priv.entries[stack->priv.depth - 1];
}

uint8_t ink_scene_depth(const ink_scene_stack_t *stack) {
    if (stack == nullptr) { return 0; }
    return stack->priv.depth;
}

const ink_scene_entry_t *ink_scene_entry_at(const ink_scene_stack_t *stack, uint8_t index) {
    if (stack == nullptr || index >= stack->priv.depth) { return nullptr; }
    return &stack->priv.entries[index];
}

void ink_scene_save(const ink_scene_stack_t *stack, ink_scene_snapshot_t *out) {
    if (stack == nullptr || out == nullptr) { return; }
    memset(out, 0, sizeof(*out)); /* unused entries are zero, so the bytes depend only on the used entries */
    out->depth = stack->priv.depth;
    for (uint8_t i = 0; i < stack->priv.depth && i < INK_SCENE_STACK_MAX; i++) { out->entries[i] = stack->priv.entries[i]; }
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
