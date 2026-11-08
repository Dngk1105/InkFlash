/* Design: docs/design/display/display.md */
#pragma once

/*
 * Sole owner of the panel. The main task acquires a frame, draws, and submits it with an intent; the display task
 * always draws the latest frame (pending frames in between are dropped), picks preset and profile, runs cleanup
 * while idle, and turns analog power off. Frame layout as in ink_hal_display.h; size from driver capabilities.
 */
#include "esp_err.h"
#include "ink_display_policy.h"
#include "ink_hal_display.h"
#include <stddef.h>
#include <stdint.h>

/* ---- Flags of a frame (intent, cleanup, action, rect: ink_display_types.h) ---- */

#define INK_DISPLAY_FLAG_DEFER_CLEANUP INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP
#define INK_DISPLAY_FLAG_BEFORE_SLEEP  INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP

/* ---- Options ---- */

/** Preset of one draw: profile per temperature band and draw config. */
typedef struct {
    ink_hal_display_band_t        bands[INK_HAL_DISPLAY_MAX_BANDS]; /**< checked in order; unknown profile id -> driver default */
    uint8_t                       band_count;
    ink_hal_display_draw_config_t config;
} ink_display_preset_t;

/** Preset of a frame draw; only frame draws add debt and set edge flags. */
typedef struct {
    ink_display_preset_t preset;
    uint8_t              debt_weight; /**< debt added per changed pixel */
    bool                 marks_edges; /**< frames drawn with this preset set edge flags */
} ink_display_draw_preset_t;

/** Options changeable at run time.
 * WARN: stored by array index; ink_display_intent_t and ink_display_cleanup_t are append only. */
typedef struct {
    ink_display_draw_preset_t draw_presets[INK_DISPLAY_INTENT_COUNT];     /**< AUTO unused */
    ink_display_preset_t      cleanup_presets[INK_DISPLAY_CLEANUP_COUNT]; /**< INVALID unused */
    uint8_t                   auto_ui_percent;
    uint8_t                   debt_light;
    uint8_t                   debt_page;
    uint8_t                   debt_ceiling;
    uint8_t                   debt_ceiling_max;
    uint32_t                  active_ms; /**< see ink_display_policy_config_t */
    uint32_t                  edge_ms;   /**< cleanup stage 1 delay; 0 = skip */
    uint32_t                  light_ms;
    uint32_t                  strong_ms;
    uint32_t                  power_off_ms; /**< no DRF for this long -> analog power off; 0 = only before sleep */
} ink_display_options_t;

/* ---- Report ---- */

/** One piece of work by the display task; timestamps from esp_timer, µs. */
typedef struct {
    esp_err_t                error;    /**< ESP_OK, or the policy or driver error */
    ink_display_action_t     action;   /**< NONE, DRAW, CLEANUP or POWER_OFF */
    bool                     is_frame; /**< draw of a submitted frame; false = cleanup or power off */
    ink_display_intent_t     intent;   /**< DRAW: resolved intent */
    ink_display_cleanup_t    cleanup;  /**< CLEANUP: kind of cleanup; INVALID otherwise */
    uint16_t                 profile_id;
    ink_display_rect_t       window;
    int8_t                   temperature_celsius; /**< valid only when temperature_valid */
    bool                     temperature_valid;   /**< sensor just read (before cleanup strong, cleanup fast) */
    uint8_t                  max_debt;            /**< after this work */
    uint8_t                  ceiling;
    uint32_t                 flagged_count;
    uint32_t                 input_ms;   /**< input time passed to submit, from ink_platform_now_ms() */
    int64_t                  acquire_us; /**< frame acquired */
    int64_t                  submit_us;  /**< frame submitted */
    int64_t                  take_us;    /**< display task took the frame, or the cleanup started */
    int64_t                  decide_us;  /**< decide and old plane done */
    ink_hal_display_timing_t driver;     /**< timing returned by the driver */
} ink_display_report_t;

/** Runs on the display task right after each piece of work; must not block. */
typedef void (*ink_display_report_callback_t)(const ink_display_report_t *report, void *user_data);

typedef struct {
    const ink_hal_display_device_t *device;     /**< copied */
    const uint8_t                  *background; /**< background frame drawn before deep sleep; NULL on cold boot */
    const ink_display_options_t    *options;    /**< nullptr = ink_display_default_options() */
    ink_display_report_callback_t   on_report;  /**< may be NULL */
    void                           *user_data;
} ink_display_config_t;

/* ---- API ---- */

/**
 * @brief Default options: presets from the driver's draw and cleanup defaults; weights, thresholds and times are display
 *        constants.
 * @par Called from
 * Any task.
 * @return Default options; empty presets if capabilities is NULL.
 */
ink_display_options_t ink_display_default_options(const ink_hal_display_capabilities_t *capabilities);

/**
 * @brief Allocate frames in internal RAM and pixel_state in PSRAM, create the display task. The background frame
 *        (if any) becomes the shown frame.
 * @par Called from
 * Main task, once at boot.
 * @retval ESP_ERR_INVALID_STATE already started.
 * @retval ESP_ERR_INVALID_ARG config is NULL, device has no operations, or capabilities or options are invalid.
 * @retval ESP_ERR_NO_MEM memory or task allocation failed.
 */
esp_err_t ink_display_start(const ink_display_config_t *config);

/**
 * @brief Change options; the display task applies them before the next frame.
 * @par Called from
 * Main task.
 * @retval ESP_ERR_INVALID_STATE not started.
 * @retval ESP_ERR_INVALID_ARG options is NULL or invalid (band count, weights, debt thresholds).
 */
esp_err_t ink_display_set_options(const ink_display_options_t *options);

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
