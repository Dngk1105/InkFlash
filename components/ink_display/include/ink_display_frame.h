/* Design: docs/design/display/display.md */
#pragma once

/*
 * Contract between the app and display: frame info, a frame, an intent and flags. Nothing here names a waveform,
 * a profile or a window, so apps and ui stay unchanged when the panel or its controller changes.
 */
#include "esp_err.h"
#include "ink_display_types.h"
#include <stdint.h>

/* ---- Flags of a frame ---- */
/* NOTE: same values as INK_DISPLAY_POLICY_FLAG_* (static_assert in ink_display.c); repeated here so this header does
 * not pull in the policy. */

#define INK_DISPLAY_FLAG_DEFER_CLEANUP (1 << 0) /**< no cleanup stage until a frame without this flag */
#define INK_DISPLAY_FLAG_BEFORE_SLEEP  (1 << 1) /**< background frame before sleep */
#define INK_DISPLAY_FLAG_REDRAW_WINDOW (1 << 2) /**< a layer opened or closed over the content */

/* ---- API ---- */

/**
 * @brief Size and pixel format of every frame.
 * @par Called from
 * Main task, after ink_display_start().
 * @retval ESP_ERR_INVALID_STATE not started.
 * @retval ESP_ERR_INVALID_ARG out_info is NULL.
 */
esp_err_t ink_display_frame_info(ink_display_frame_info_t *out_info);

/**
 * @brief Acquire a frame to draw into. It already holds the latest submitted image (white if none yet).
 * @par Called from
 * Main task.
 * @return Frame, or NULL if not started or the previous frame is acquired and not submitted.
 */
uint8_t *ink_display_acquire(void);

/**
 * @brief Submit the acquired frame with its intent and flags; does not wait.
 * @par Called from
 * Main task.
 * @param input_ms Input time from ink_platform_now_ms(); copied into the report.
 * @param flags INK_DISPLAY_FLAG_*.
 * @retval ESP_ERR_INVALID_STATE not started, or no frame acquired.
 * @retval ESP_ERR_INVALID_ARG `frame` is not the acquired frame, or intent is out of range.
 */
esp_err_t ink_display_submit(uint8_t *frame, uint32_t input_ms, ink_display_intent_t intent, uint8_t flags);
