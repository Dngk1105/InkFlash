/* Design: docs/design/app-framework.md */
#pragma once

/*
 * Scene stack of one app: up to INK_SCENE_STACK_MAX entries, each a scene id and INK_SCENE_STATE_BYTES bytes of scene
 * state (plain numbers, no pointers, so a snapshot can be kept in RTC memory across deep sleep). The block only moves
 * entries; it never calls a scene. Pure C (C23).
 */
#include <stdint.h>

#define INK_SCENE_STACK_MAX   4 /**< depth of the stack, root included */
#define INK_SCENE_STATE_BYTES 8 /**< scene state bytes in one entry */

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_SCENE_OK = 0,
    INK_SCENE_ERROR_ARGUMENT, /**< a pointer is NULL */
    INK_SCENE_ERROR_FULL,     /**< push at INK_SCENE_STACK_MAX */
    INK_SCENE_ERROR_EMPTY,    /**< pop with only the root left */
    INK_SCENE_ERROR_SNAPSHOT, /**< snapshot depth is not 1..INK_SCENE_STACK_MAX, or a scene id is out of range */
} ink_scene_error_t;

/* ---- Stack ---- */

/** One stack level. */
typedef struct {
    uint8_t scene_id;
    uint8_t state[INK_SCENE_STATE_BYTES]; /**< owned by the scene; written whenever the scene state changes */
} ink_scene_entry_t;

/** Copy of a stack for RTC memory. Plain numbers only; the same layout on every build. */
typedef struct {
    uint8_t           depth;
    ink_scene_entry_t entries[INK_SCENE_STACK_MAX];
} ink_scene_snapshot_t;

static_assert(sizeof(ink_scene_snapshot_t) <= 40, "snapshot must stay small: it lives in an RTC slot");

/** The caller owns the struct but must not touch `priv`. */
typedef struct {
    struct {
        uint8_t           depth; /**< 1..INK_SCENE_STACK_MAX after ink_scene_init() */
        ink_scene_entry_t entries[INK_SCENE_STACK_MAX];
    } priv;
} ink_scene_stack_t;

/* ---- API ---- */

/**
 * @brief Start a stack holding only the root scene, with its state zeroed.
 * @par Called from
 * Main task (system), when an app opens.
 */
void ink_scene_init(ink_scene_stack_t *stack, uint8_t root_id);

/**
 * @brief Push a scene on top.
 * @par Called from
 * Main task (system).
 * @param state INK_SCENE_STATE_BYTES bytes copied in; NULL = zeros.
 * @retval INK_SCENE_ERROR_ARGUMENT stack is NULL.
 * @retval INK_SCENE_ERROR_FULL the stack is at INK_SCENE_STACK_MAX; it is unchanged.
 */
[[nodiscard]] ink_scene_error_t ink_scene_push(ink_scene_stack_t *stack, uint8_t scene_id, const uint8_t *state);

/**
 * @brief Remove the scene on top.
 * @par Called from
 * Main task (system).
 * @retval INK_SCENE_ERROR_ARGUMENT stack is NULL.
 * @retval INK_SCENE_ERROR_EMPTY only the root is left; the root is never popped, the caller goes Home instead.
 */
[[nodiscard]] ink_scene_error_t ink_scene_pop(ink_scene_stack_t *stack);

/**
 * @brief Overwrite the scene on top; the depth stays.
 * @par Called from
 * Main task (system).
 * @param state INK_SCENE_STATE_BYTES bytes copied in; NULL = zeros.
 * @retval INK_SCENE_ERROR_ARGUMENT stack is NULL.
 */
[[nodiscard]] ink_scene_error_t ink_scene_replace(ink_scene_stack_t *stack, uint8_t scene_id, const uint8_t *state);

/**
 * @brief Drop every entry above the root; the root entry and its state are kept.
 * @par Called from
 * Main task (system).
 */
void ink_scene_reset(ink_scene_stack_t *stack);

/**
 * @brief Get the entry on top.
 * @par Called from
 * Main task (system).
 * @return The entry, valid until the next push, pop, replace or reset; NULL if stack is NULL or was never initialized (depth 0).
 */
ink_scene_entry_t *ink_scene_top(ink_scene_stack_t *stack);

/**
 * @brief Number of entries.
 * @par Called from
 * Main task (system).
 * @return 1..INK_SCENE_STACK_MAX; 0 if stack is NULL.
 */
uint8_t ink_scene_depth(const ink_scene_stack_t *stack);

/**
 * @brief Get the entry at `index`, counted from the root.
 * @par Called from
 * Main task (system), to call enter() from the bottom up after wake.
 * @param index 0 = root.
 * @return The entry; NULL if stack is NULL or index is not below the depth.
 */
const ink_scene_entry_t *ink_scene_entry_at(const ink_scene_stack_t *stack, uint8_t index);

/**
 * @brief Copy the stack into a snapshot.
 * @par Called from
 * Main task (system), before sleep or before an app closes.
 * @param out Untouched if a pointer is NULL.
 */
void ink_scene_save(const ink_scene_stack_t *stack, ink_scene_snapshot_t *out);

/**
 * @brief Rebuild the stack from a snapshot.
 * @par Called from
 * Main task (system), after wake.
 * @param scene_count Scenes of the app; every scene id in the snapshot must be below it.
 * @retval INK_SCENE_ERROR_ARGUMENT a pointer is NULL.
 * @retval INK_SCENE_ERROR_SNAPSHOT depth is not 1..INK_SCENE_STACK_MAX or a scene id is out of range; the stack is unchanged.
 */
[[nodiscard]] ink_scene_error_t ink_scene_restore(ink_scene_stack_t *stack, const ink_scene_snapshot_t *in, uint8_t scene_count);
