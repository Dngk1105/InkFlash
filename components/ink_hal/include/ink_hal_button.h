/* Design: docs/design/input/input.md */
#pragma once

/*
 * Interface between input and a button driver: one device per button. The driver takes no timestamp;
 * the receiver of the callback does.
 */
#include "esp_err.h"
#include <stdbool.h>

/** Runs in the ISR on every pin level change. WARN: must be in IRAM, must not log or block. */
typedef void (*ink_hal_button_callback_t)(void *user_data, bool pressed);

/** What the driver can do. */
typedef struct {
    bool has_interrupt; /**< reports edges by interrupt through set_callback */
} ink_hal_button_capabilities_t;

/** Driver operations; `context` is the driver's own data. */
typedef struct {
    /** Read the current level; true = pressed. Called from the main task (resync). */
    esp_err_t (*read)(void *context, bool *out_pressed);
    /** Register the callback, then enable the interrupt. Call once, from a task. */
    esp_err_t (*set_callback)(void *context, ink_hal_button_callback_t callback, void *user_data);
} ink_hal_button_operations_t;

typedef struct {
    const ink_hal_button_operations_t *operations;
    void                              *context;
    ink_hal_button_capabilities_t      capabilities;
} ink_hal_button_device_t;
