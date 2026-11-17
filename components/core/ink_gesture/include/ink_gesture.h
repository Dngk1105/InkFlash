/* Design: docs/design/input/input.md */
#pragma once

/*
 * Button edges -> gestures. Never reads the clock: the caller passes time in every call and ticks at
 * ink_gesture_next_deadline(). Pure C (C23).
 */
#include "ink_gesture_types.h"

/* ---- State ---- */

/** State of one button; used only inside ink_gesture_state_t. */
typedef struct {
    uint8_t  phase;
    uint8_t  wait_mask;
    uint8_t  modifier;         /**< modifier latched at press */
    bool     stable;           /**< level after debounce; true = pressed */
    bool     raw;              /**< level of the latest edge */
    bool     locked;           /**< within debounce_ms after an accepted edge */
    bool     used_as_modifier; /**< acting as modifier for the other button */
    uint32_t lock_until_ms;
    uint32_t pressed_at_ms;
    uint32_t released_at_ms;
} ink_gesture_button_state_t;

/** The caller owns the struct but must not touch `priv`; tools/check.py rejects `priv` outside the core module. */
typedef struct {
    struct {
        ink_gesture_config_t       config;
        ink_gesture_button_state_t buttons[INK_GESTURE_BUTTON_COUNT];
        uint8_t                    mode;
        uint32_t                   ab_pressed_at_ms; /**< press time of the second button of A+B */
    } priv;
} ink_gesture_state_t;

/* ---- API ---- */

/**
 * @brief Reset state and set the gesture config.
 * @par Called from
 * Input, at boot.
 * @retval INK_GESTURE_ERROR_ARGUMENT a pointer is NULL.
 * @retval INK_GESTURE_ERROR_CONFIG config is invalid.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_init(ink_gesture_state_t *state, const ink_gesture_config_t *config);

/**
 * @brief Change thresholds at run time; gestures in progress use the new values at once.
 * @par Called from
 * Main task (input, from the Settings app).
 * @retval INK_GESTURE_ERROR_ARGUMENT a pointer is NULL.
 * @retval INK_GESTURE_ERROR_CONFIG config is invalid; the old config stays.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_set_config(ink_gesture_state_t *state, const ink_gesture_config_t *config);

/**
 * @brief Copy the current gesture config.
 * @par Called from
 * Main task.
 * @retval INK_GESTURE_ERROR_ARGUMENT a pointer is NULL.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_get_config(const ink_gesture_state_t *state, ink_gesture_config_t *out_config);

/**
 * @brief Set the wait mask of one button.
 *
 * A changed mask drops the gesture in progress on that button; the button must be released before the next gesture.
 * @par Called from
 * Main task (system, when an app opens or the scene changes).
 * @param wait_mask INK_GESTURE_WAIT_* bits.
 * @retval INK_GESTURE_ERROR_ARGUMENT state is NULL, button is not A or B, or wait_mask has unknown bits.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_set_wait_mask(ink_gesture_state_t *state, ink_gesture_button_t button,
                                                            ink_gesture_wait_mask_t wait_mask);

/**
 * @brief Feed one edge.
 *
 * Runs deadlines due before event->time_ms first, so gestures come out in time order.
 * @par Called from
 * Main task, on every edge.
 * @param out Gestures emitted; out->count may be 0.
 * @retval INK_GESTURE_ERROR_ARGUMENT a pointer is NULL, button is not A or B, or level is unknown.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_feed(ink_gesture_state_t *state, const ink_gesture_event_t *event, ink_gesture_output_t *out);

/**
 * @brief Run every deadline due at now_ms.
 * @par Called from
 * Main task, when the timer set from ink_gesture_next_deadline() expires.
 * @retval INK_GESTURE_ERROR_ARGUMENT a pointer is NULL.
 */
[[nodiscard]] ink_gesture_error_t ink_gesture_tick(ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_output_t *out);

/**
 * @brief Get the earliest deadline, to set the timer.
 * @par Called from
 * Main task, after every feed and tick.
 * @return false if no deadline is pending (no timer needed), or a pointer is NULL.
 */
bool ink_gesture_next_deadline(const ink_gesture_state_t *state, uint32_t *out_ms);

/**
 * @brief Convert a button index (0 = A, 1 = B; the order bsp lists buttons in) to its button code.
 * @par Called from
 * Main task (input).
 * @return Button code; 0 if index >= INK_GESTURE_BUTTON_COUNT.
 */
ink_gesture_button_t ink_gesture_button_of_index(uint8_t index);

/**
 * @brief Convert a button code to its index (A -> 0, B -> 1).
 * @par Called from
 * Main task (input, keymap).
 * @return Index; -1 unless button is exactly one known button.
 */
int8_t ink_gesture_index_of_button(ink_gesture_button_t button);

/**
 * @brief Report the gesture being held at now_ms. Read only; emits nothing.
 * @par Called from
 * Main task.
 * @return false if nothing is held, or a pointer is NULL.
 */
bool ink_gesture_progress(const ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_progress_t *out);
