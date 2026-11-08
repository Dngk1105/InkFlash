/* Design: docs/design/tasks.md */
#pragma once

/*
 * Main task: holds state, receives messages from the main queue and hands each to its owner module.
 * ISRs and timer callbacks of every module only post messages to this queue.
 */
#include "esp_err.h"
#include "ink_display.h"
#include "ink_hal.h"
#include <stddef.h>

typedef struct {
    const ink_hal_button_device_t  *buttons; /**< index 0 = A, 1 = B; must live for the whole program (input resyncs from it) */
    size_t                          button_count;
    const ink_hal_display_device_t *display;
    const ink_display_options_t    *display_options; /**< stored display options; NULL = defaults */
} ink_system_config_t;

/**
 * @brief Create the main queue and the main task, then start input (button interrupts on) and display.
 * @par Called from
 * app_main, once, after ink_bsp_init().
 * @retval ESP_ERR_INVALID_ARG config or display is NULL, button_count is not 2, or display options are invalid.
 * @retval ESP_ERR_NOT_SUPPORTED a button has no interrupt.
 * @retval ESP_ERR_INVALID_STATE already started.
 * @retval ESP_ERR_NO_MEM the main queue, a task or the frames could not be allocated.
 */
esp_err_t ink_system_start(const ink_system_config_t *config);
