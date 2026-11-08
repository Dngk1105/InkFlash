/* Design: docs/design/code-structure.md */
#pragma once

/*
 * Template of a pure C block (C23): takes events, returns actions. Includes nothing from
 * ESP-IDF or FreeRTOS. State lives in a struct owned by the caller (the shell keeps it as a
 * static variable), so host tests can create many instances. This example counts TICK events;
 * after `limit` ticks it returns NOTIFY and starts counting again.
 */
#include <stdint.h>

/* ---- Errors ---- */

/** Errors of this block. List only errors the block really returns. */
typedef enum : uint8_t {
    INK_EXAMPLE_OK = 0,
    INK_EXAMPLE_ERROR_ARGUMENT,
} ink_example_error_t;

/* ---- Events in, actions out ---- */

/** Event type.
 * NOTE: fixed 1-byte underlying type, so the size stays the same when stored in flash or RTC memory. */
typedef enum : uint8_t {
    INK_EXAMPLE_EVENT_TICK,
    INK_EXAMPLE_EVENT_RESET,
} ink_example_event_type_t;

typedef struct {
    ink_example_event_type_t type;
    uint32_t                 time_ms; /**< passed in by the caller; the block never reads a clock */
} ink_example_event_t;

typedef enum : uint8_t {
    INK_EXAMPLE_ACTION_NONE,
    INK_EXAMPLE_ACTION_NOTIFY,
} ink_example_action_t;

/* ---- State ---- */

/** The caller owns the struct but must not touch its fields: all of them are in `priv`, and
 * tools/check.py rejects `priv` access outside the core block's folder. */
typedef struct {
    struct {
        uint32_t count;
        uint32_t limit;
    } priv;
} ink_example_state_t;

/* ---- API ---- */

/**
 * @brief Reset the state.
 * @par Called from
 * Shell, once at boot.
 * @retval INK_EXAMPLE_ERROR_ARGUMENT state is NULL or limit is 0.
 */
[[nodiscard]] ink_example_error_t ink_example_init(ink_example_state_t *state, uint32_t limit);

/**
 * @brief Handle one event; write the action to take into `out_action`.
 * @par Called from
 * Shell task, once per event.
 * @retval INK_EXAMPLE_ERROR_ARGUMENT a pointer is NULL or the event type is invalid.
 */
[[nodiscard]] ink_example_error_t ink_example_handle(ink_example_state_t *state, const ink_example_event_t *event,
                                                     ink_example_action_t *out_action);
