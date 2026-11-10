/* Design: docs/design/ui.md */
#pragma once

/*
 * HACK(demo): test screens for ink_ui on the board, until the first app exists; then remove this component and its
 * calls in ink_system. Runs on the main task and holds its own state. Button A (PRESS) shows the next screen,
 * button B (PRESS) the previous one.
 */
#include "esp_err.h"
#include "ink_gesture.h"

/* ---- API ---- */

/**
 * @brief Set up the canvas and draw the first test screen.
 * @par Called from
 * Main task, once, after ink_display_start().
 * @retval ESP_ERR_INVALID_STATE display is not started.
 * @retval ESP_ERR_NOT_SUPPORTED ui cannot draw on the frames of this panel.
 */
esp_err_t ink_demo_start(void);

/**
 * @brief Change the test screen according to the gestures just emitted.
 * @par Called from
 * Main task.
 * @param gestures May be NULL or empty.
 */
void ink_demo_handle(const ink_gesture_output_t *gestures);
