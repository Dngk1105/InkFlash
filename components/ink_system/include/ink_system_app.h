/* Design: docs/design/app-framework.md */
#pragma once

/*
 * Interface between system and the apps: what an app declares (ink_system_app_t, ink_system_scene_t) and what a scene
 * may call back (scene stack operations, open another app, keep awake). Everything here runs on the main task.
 * An app uses only this header, display, ui and study; it never touches the scene stack or the queue directly.
 */
#include "esp_err.h"
#include "ink_keymap.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INK_SYSTEM_KEEP_AWAKE_MAX_MS 30000 /**< longest wake extension of one ink_system_keep_awake() call, ms */

/* ---- Declared by the app ---- */

/** One scene: three functions over the 8 byte state of its stack entry (INK_SCENE_STATE_BYTES). */
typedef struct {
    /** Scene was pushed or replaced in (restored = false) or rebuilt from a snapshot after wake (restored = true).
     * `state` holds what the caller of push/replace gave, or zeros. Must not draw. */
    void (*enter)(uint8_t *state, bool restored);
    /** A command for the scene on top. Returns true if handled; an unhandled BACK pops the scene. */
    bool (*command)(uint8_t *state, ink_keymap_command_t command);
    /** Redraw the whole screen (acquire, draw with ui, submit). Small changes inside `command` draw by themselves. */
    void (*draw)(const uint8_t *state);
    const ink_keymap_table_t *keymap; /**< scene table: only the entries that differ from the app table; nullptr = none */
} ink_system_scene_t;

/** One app. The list of apps is fixed at build time; `scenes[0]` is the root scene. */
typedef struct {
    const char *name; /**< shown by Home */
    esp_err_t (*start)(const uint8_t *snapshot,
                       size_t         length);              /**< acquire shared resources; empty snapshot = fresh open; may be nullptr */
    void (*save)(uint8_t *snapshot, size_t length); /**< write app-level state before sleep or close; may be nullptr */
    void (*stop)(void);                             /**< release everything `start` acquired; may be nullptr */
    const ink_keymap_table_t *app_table;            /**< default gestures of the app's commands; must map BACK */
    const ink_system_scene_t *scenes;               /**< all scenes of the app, indexed by scene id */
    uint8_t                   scene_count;
} ink_system_app_t;

/* ---- Called by scenes ---- */

/**
 * @brief Push a scene on the stack, call its enter() with restored = false, then its draw().
 * @par Called from
 * Main task, from a scene function.
 * @param state INK_SCENE_STATE_BYTES bytes copied into the new entry; NULL = zeros.
 * @retval ESP_ERR_INVALID_ARG scene_id is not a scene of the open app.
 * @retval ESP_ERR_NO_MEM the stack is full; it is unchanged and the caller handles the error.
 */
[[nodiscard]] esp_err_t ink_system_scene_push(uint8_t scene_id, const uint8_t *state);

/**
 * @brief Pop the scene on top, then call draw() of the scene now on top.
 * @par Called from
 * Main task, from a scene function.
 * @retval ESP_ERR_INVALID_STATE only the root scene is left; it is never popped.
 */
[[nodiscard]] esp_err_t ink_system_scene_pop(void);

/**
 * @brief Replace the scene on top (same depth), call its enter() with restored = false, then its draw().
 * @par Called from
 * Main task, from a scene function.
 * @param state INK_SCENE_STATE_BYTES bytes copied into the entry; NULL = zeros.
 * @retval ESP_ERR_INVALID_ARG scene_id is not a scene of the open app.
 */
[[nodiscard]] esp_err_t ink_system_scene_replace(uint8_t scene_id, const uint8_t *state);

/**
 * @brief Drop every scene above the root, then call draw() of the root scene (its state is kept).
 * @par Called from
 * Main task, from a scene function.
 */
void ink_system_scene_reset(void);

/**
 * @brief Close the open app and open app `app_index` with a fresh scene stack (root scene, state zeros).
 * @par Called from
 * Main task (Home, system).
 * @retval ESP_ERR_INVALID_ARG app_index is out of range.
 * @return Other codes come from the start() of the app; Home is opened instead, unless Home itself failed.
 */
[[nodiscard]] esp_err_t ink_system_open_app(uint8_t app_index);

/**
 * @brief Number of apps; app 0 is Home.
 * @par Called from
 * Main task.
 */
uint8_t ink_system_app_count(void);

/**
 * @brief Get an app by index.
 * @par Called from
 * Main task.
 * @return The app, or NULL if app_index is out of range.
 */
const ink_system_app_t *ink_system_app_at(uint8_t app_index);

/**
 * @brief Ask system not to sleep for `ms` (for example while playing audio).
 * @par Called from
 * Main task.
 * @param ms Capped at INK_SYSTEM_KEEP_AWAKE_MAX_MS; ask again to keep going.
 */
void ink_system_keep_awake(uint32_t ms);

/**
 * @brief Call `next` once, on the main task, when the next display report arrives (second phase of a two-phase draw:
 *        UI frame first, TEXT frame after the display task took the first). Cleared when the scene changes.
 * @par Called from
 * Main task, from a scene function.
 * @param next Receives the state of the scene on top; NULL cancels.
 */
void ink_system_after_report(void (*next)(const uint8_t *state));
