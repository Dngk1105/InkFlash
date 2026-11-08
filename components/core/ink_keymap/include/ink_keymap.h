/* Design: docs/design/input/input.md */
#pragma once

/*
 * Gesture -> keymap intent through the scene table and the app table. Stateless: system owns the tables.
 * Pure C (C23).
 */
#include "ink_gesture.h"

#include <stdint.h>

/* ---- Keymap intent ---- */
/*
 * WARN: keymap tables store these as numbers. Shared intents take only the free codes 4..15; app intents start at
 * INK_KEYMAP_INTENT_APP_BASE, e.g. (ink_keymap_intent_t)(INK_KEYMAP_INTENT_APP_BASE + 1).
 */

typedef enum : uint8_t {
    INK_KEYMAP_INTENT_NEXT     = 0,
    INK_KEYMAP_INTENT_PREV     = 1,
    INK_KEYMAP_INTENT_SELECT   = 2,
    INK_KEYMAP_INTENT_BACK     = 3,
    INK_KEYMAP_INTENT_APP_BASE = 16,
} ink_keymap_intent_t;

/* ---- Keymap table ---- */

#define INK_KEYMAP_ENTRY_MAX 32

typedef struct {
    ink_gesture_t       gesture;
    ink_keymap_intent_t intent;
} ink_keymap_entry_t;

/** App table, or scene table (only the entries that differ from the app table). The caller owns and edits it. */
typedef struct {
    uint8_t            count;
    ink_keymap_entry_t entries[INK_KEYMAP_ENTRY_MAX];
} ink_keymap_table_t;

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_KEYMAP_OK = 0,
    INK_KEYMAP_ERROR_ARGUMENT,
    INK_KEYMAP_ERROR_COUNT,     /**< count exceeds INK_KEYMAP_ENTRY_MAX */
    INK_KEYMAP_ERROR_RESERVED,  /**< an entry maps HOLD_AB_SHORT or HOLD_AB_LONG (reserved for system) */
    INK_KEYMAP_ERROR_DUPLICATE, /**< one gesture in two entries of the same table */
    INK_KEYMAP_ERROR_NO_BACK,   /**< no gesture looks up to BACK */
} ink_keymap_error_t;

/* ---- API ---- */

/**
 * @brief Look up the keymap intent of a gesture: scene table first, then app table.
 * @par Called from
 * Main task (system), for every gesture except HOLD_AB_SHORT and HOLD_AB_LONG.
 * @param scene May be NULL.
 * @return false if neither table maps the gesture (the gesture is dropped), or a pointer is NULL.
 */
bool ink_keymap_lookup(const ink_keymap_table_t *scene, const ink_keymap_table_t *app, const ink_gesture_t *gesture,
                       ink_keymap_intent_t *out_intent);

/**
 * @brief Validate the effective table (scene table over app table).
 * @par Called from
 * Main task (system), before accepting a table the user edited, and when an app declares a scene table.
 * @param scene May be NULL.
 * @retval INK_KEYMAP_ERROR_ARGUMENT app is NULL.
 * @retval INK_KEYMAP_ERROR_COUNT a table has count > INK_KEYMAP_ENTRY_MAX.
 * @retval INK_KEYMAP_ERROR_RESERVED a table maps HOLD_AB_SHORT or HOLD_AB_LONG.
 * @retval INK_KEYMAP_ERROR_DUPLICATE a table has the same gesture twice.
 * @retval INK_KEYMAP_ERROR_NO_BACK no gesture looks up to BACK.
 */
[[nodiscard]] ink_keymap_error_t ink_keymap_validate(const ink_keymap_table_t *scene, const ink_keymap_table_t *app);

/**
 * @brief Copy the default table into the table in use.
 * @par Called from
 * Main task (Settings app, "reset keymap").
 * @retval INK_KEYMAP_ERROR_ARGUMENT a pointer is NULL.
 */
[[nodiscard]] ink_keymap_error_t ink_keymap_reset(ink_keymap_table_t *table, const ink_keymap_table_t *defaults);

/**
 * @brief Compute the wait mask of one button from the tables in use.
 *
 * Counts app entries the scene overrides too, so the mask may enable more than needed, never less.
 * @par Called from
 * Main task (system), to pass to ink_input_set_wait_mask() when an app opens or the scene changes.
 * @return INK_GESTURE_WAIT_* bits; INK_GESTURE_WAIT_NONE if button is not A or B.
 */
ink_gesture_wait_mask_t ink_keymap_wait_mask(const ink_keymap_table_t *scene, const ink_keymap_table_t *app, ink_gesture_button_t button);
