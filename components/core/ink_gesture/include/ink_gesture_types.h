/* Design: docs/design/input/input.md */
#pragma once

/*
 * Types shared by gesture, keymap, input and callers. Types only, no logic, no ESP-IDF include.
 * Pure C (C23).
 */
#include <stdbool.h>
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
 * WARN: ink_gesture.c pairs buttons as `1 - index` (static_assert there); a third button needs that code rewritten. */
#define INK_GESTURE_BUTTON_COUNT 2

typedef enum : uint8_t {
    INK_GESTURE_TYPE_PRESS = 0,
    INK_GESTURE_TYPE_DOUBLE_PRESS,
    INK_GESTURE_TYPE_HOLD,
    INK_GESTURE_TYPE_HOLD_AB_SHORT, /**< reserved for system: back to menu */
    INK_GESTURE_TYPE_HOLD_AB_LONG,  /**< reserved for system: power off */
    INK_GESTURE_TYPE_HOLD_SHORT,    /**< held between hold_short_ms and hold_ms */
    INK_GESTURE_TYPE_HOLD_LONG,     /**< held until hold_long_ms */
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
    uint32_t hold_short_ms; /**< short hold threshold, ms */
    uint32_t hold_ms;       /**< default hold threshold, ms */
    uint32_t hold_long_ms;  /**< long hold threshold, ms */
    uint32_t combo_ms;      /**< second button within this time after the first: A+B */
    uint32_t ab_short_ms;
    uint32_t ab_long_ms;
} ink_gesture_config_t;

/** Wait mask bits for ink_gesture_set_wait_mask(): gesture types that make a button wait. All off by default. */
typedef enum : uint8_t {
    INK_GESTURE_WAIT_NONE     = 0,
    INK_GESTURE_WAIT_DOUBLE   = 1 << 0, /**< PRESS waits double_ms after release for a second press */
    INK_GESTURE_WAIT_MODIFIER = 1 << 1, /**< HOLD waits for release: the button may become a modifier */
} ink_gesture_wait_mask_t;
