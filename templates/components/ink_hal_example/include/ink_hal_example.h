/* Design: docs/design/hardware-variants.md */
#pragma once

/*
 * Template of a hardware interface (HAL). Upper layers see only the operations and the
 * capabilities, never the chip name. Each driver fills one ink_hal_example_device_t.
 */
#include "esp_err.h"
#include <stdbool.h>

/** Capabilities: what the driver can do. Add a field only when an upper layer really uses it. */
typedef struct {
    bool has_interrupt; /**< reports changes by interrupt, no polling needed */
} ink_hal_example_capabilities_t;

/** Operations. `context` is the driver's private data. */
typedef struct {
    /**
     * @brief Read the input level.
     * @par Called from
     * Any task.
     * @return ESP_OK, or a driver error.
     */
    esp_err_t (*read)(void *context, bool *out_active);
} ink_hal_example_operations_t;

typedef struct {
    const ink_hal_example_operations_t *operations;
    void                               *context;
    ink_hal_example_capabilities_t      capabilities;
} ink_hal_example_device_t;
