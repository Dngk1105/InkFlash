/* Design: docs/design/input/input.md */
#pragma once

/*
 * Button edges -> gestures. Never reads the clock: the caller passes time in every call and ticks at
 * ink_gesture_next_deadline(). Pure C (C23).
 */
#include <stdint.h>

/* ---- Gesture codes ---- */
/*
 * WARN: append only to every enum below; keymap tables store gestures as numbers, so reordering silently
 * points stored entries at other gestures.
 * Button and modifier codes are bit masks: one bit per button, a combination is the OR of the bits.
 */

typedef enum : uint8_t {
    INK_GESTURE_SOURCE_BUTTON = 0,
    /* EXTEND(gesture): add IMU here; see the IMU section of input.md. */
} ink_gesture_source_t;

/** Button code. An input edge names one button; only output gestures use combinations. */
typedef enum : uint8_t {
    INK_GESTURE_BUTTON_A  = 1 << 0,
    INK_GESTURE_BUTTON_B  = 1 << 1,
    INK_GESTURE_BUTTON_AB = INK_GESTURE_BUTTON_A | INK_GESTURE_BUTTON_B,
} ink_gesture_button_t;

/* Number of buttons gesture handles; the button codes above use exactly this many bits.
 * WARN: ink_gesture.c pairs buttons as `1 - index`; a third button needs that code rewritten, not just this number. */
#define INK_GESTURE_BUTTON_COUNT 2

typedef enum : uint8_t {
    INK_GESTURE_TYPE_PRESS = 0,
    INK_GESTURE_TYPE_DOUBLE_PRESS,
    INK_GESTURE_TYPE_HOLD,
    INK_GESTURE_TYPE_HOLD_AB_SHORT, /**< reserved for system: back to menu */
    INK_GESTURE_TYPE_HOLD_AB_LONG,  /**< reserved for system: power off */
} ink_gesture_type_t;

/** Button held when the trigger started. */
typedef enum : uint8_t {
    INK_GESTURE_MODIFIER_NONE = 0,
    INK_GESTURE_MODIFIER_A    = INK_GESTURE_BUTTON_A,
    INK_GESTURE_MODIFIER_B    = INK_GESTURE_BUTTON_B,
} ink_gesture_modifier_t;

/** One gesture: (modifier, trigger); the trigger is (source, button, type). */
typedef struct {
    ink_gesture_modifier_t modifier;
    ink_gesture_source_t   source;
    ink_gesture_button_t   button;
    ink_gesture_type_t     type;
} ink_gesture_t;

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_GESTURE_OK = 0,
    INK_GESTURE_ERROR_ARGUMENT,
    INK_GESTURE_ERROR_CONFIG, /**< thresholds violate the rules in config_valid() */
} ink_gesture_error_t;

/* ---- Input: button edges ---- */

typedef enum : uint8_t {
    INK_GESTURE_LEVEL_RELEASED = 0,
    INK_GESTURE_LEVEL_PRESSED,
} ink_gesture_level_t;

/** One edge. */
typedef struct {
    ink_gesture_button_t button;  /**< exactly one button: A or B */
    ink_gesture_level_t  level;   /**< level after the edge */
    uint32_t             time_ms; /**< timestamp taken by the caller */
} ink_gesture_event_t;

/* ---- Output ---- */

#define INK_GESTURE_OUTPUT_MAX 4

typedef struct {
    uint8_t       count;
    ink_gesture_t items[INK_GESTURE_OUTPUT_MAX];
} ink_gesture_output_t;

/** Gesture being held, so system can draw a countdown. */
typedef struct {
    ink_gesture_modifier_t modifier;
    ink_gesture_button_t   button;
    ink_gesture_type_t     on_release; /**< gesture emitted if released now */
    bool                   has_next;
    ink_gesture_type_t     next; /**< gesture reached by holding until next_ms */
    uint32_t               start_ms;
    uint32_t               next_ms;
} ink_gesture_progress_t;

/* ---- Config ---- */

/** Time thresholds, ms. */
typedef struct {
    uint32_t debounce_ms;
    uint32_t double_ms;
    uint32_t hold_ms;
    uint32_t combo_ms; /**< second button within this time after the first: A+B */
    uint32_t ab_short_ms;
    uint32_t ab_long_ms;
} ink_gesture_config_t;

/** Wait mask bits for ink_gesture_set_enabled(): gesture types that make a button wait. All off by default. */
typedef enum : uint8_t {
    INK_GESTURE_WAIT_NONE     = 0,
    INK_GESTURE_WAIT_DOUBLE   = 1 << 0, /**< PRESS waits double_ms after release for a second press */
    INK_GESTURE_WAIT_MODIFIER = 1 << 1, /**< HOLD waits for release: the button may become a modifier */
} ink_gesture_wait_mask_t;

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
[[nodiscard]] ink_gesture_error_t ink_gesture_set_enabled(ink_gesture_state_t *state, ink_gesture_button_t button, uint8_t wait_mask);

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
 * @brief Report the gesture being held at now_ms. Read only; emits nothing.
 * @par Called from
 * Main task.
 * @return false if nothing is held, or a pointer is NULL.
 */
bool ink_gesture_progress(const ink_gesture_state_t *state, uint32_t now_ms, ink_gesture_progress_t *out);
