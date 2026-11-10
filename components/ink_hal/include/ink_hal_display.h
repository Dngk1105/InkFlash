/* Design: docs/design/display/display.md */
#pragma once

/*
 * Interface between display and the panel driver.
 * Frame layout: capabilities.pixel_format (ink_display_types.h).
 * Coordinates in native panel orientation, px. Only the display task calls `operations`.
 */
#include "esp_err.h"
#include "ink_display_types.h"
#include <stddef.h>
#include <stdint.h>

#define INK_HAL_DISPLAY_SOURCE_CUSTOM 0xFF /**< custom profile; 0-8 = bands TR0-TR8 copied from OTP */
#define INK_HAL_DISPLAY_MAX_BANDS     9

/** Per-draw config, kept apart from the profile so one waveform serves several presets. */
typedef struct {
    bool partial_mode;        /**< partial mode (PTL) on window; false = whole panel, window ignored */
    bool scan_outside_window; /**< partial mode: scan gates outside the window too (UC8253 PT_SCAN) */
    bool drive_border;        /**< drive the border with the waveform; false = border floats */
} ink_hal_display_draw_config_t;

typedef struct {
    uint32_t spi_us;       /**< send image and waveform, before the scan */
    uint32_t power_us;     /**< analog power on (PON) if it was off; 0 if already on */
    uint32_t panel_us;     /**< panel scan until BUSY goes high (idle), excluding power_us */
    uint32_t post_scan_us; /**< work after the scan */
} ink_hal_display_timing_t;

typedef struct {
    uint16_t    id; /**< stable across builds; options store the id */
    const char *name;
    uint8_t     source;  /**< 0–8 = TR0–TR8, INK_HAL_DISPLAY_SOURCE_CUSTOM */
    uint32_t    draw_ms; /**< one draw (all scans), computed by tools/profile_gen.py */
} ink_hal_display_profile_info_t;

/** One band: temperature <= max_celsius -> use profile_id. */
typedef struct {
    int8_t   max_celsius;
    uint16_t profile_id;
} ink_hal_display_band_t;

/** Driver default preset for one intent or one kind of cleanup. */
typedef struct {
    const ink_hal_display_band_t *bands; /**< checked in order; above the last band -> last band */
    uint8_t                       band_count;
    ink_hal_display_draw_config_t config;
} ink_hal_display_preset_default_t;

typedef struct {
    uint16_t                                width; /**< native panel width, px */
    uint16_t                                height;
    ink_display_pixel_format_t              pixel_format;
    uint8_t                                 x_align; /**< window left/right edges are multiples of this */
    const ink_hal_display_profile_info_t   *profiles;
    size_t                                  profile_count;
    const ink_hal_display_preset_default_t *draw_defaults;    /**< INK_DISPLAY_INTENT_COUNT entries; AUTO unused */
    const ink_hal_display_preset_default_t *cleanup_defaults; /**< INK_DISPLAY_CLEANUP_COUNT entries; INVALID unused */
} ink_hal_display_capabilities_t;

typedef struct {
    /**
     * @brief Draw window from the (new, old) bit pair of each pixel with profile `profile_index`; wait for the scan.
     *
     * Turns analog power on if off and leaves it on. The driver rounds x edges out to x_align and widens to the
     * whole panel on the first draw after reset.
     * @par Called from
     * Display task.
     * @param plane_new, plane_old Whole frames, in internal RAM.
     * @param profile_index Index into capabilities.profiles (not the profile id).
     * @retval ESP_ERR_INVALID_ARG bad argument or profile_index out of range.
     * @retval ESP_ERR_TIMEOUT BUSY did not go high.
     * @retval other SPI error.
     */
    esp_err_t (*draw)(void *context, const uint8_t *plane_new, const uint8_t *plane_old, ink_display_rect_t window, size_t profile_index,
                      const ink_hal_display_draw_config_t *config, ink_hal_display_timing_t *out_timing);
    /**
     * @brief Turn analog power off if on.
     * @par Called from
     * Display task.
     * @retval ESP_ERR_TIMEOUT BUSY did not go high.
     * @retval other SPI error.
     */
    esp_err_t (*power_off)(void *context);
    /**
     * @brief Read the controller temperature sensor, °C.
     * @par Called from
     * Display task.
     * @retval ESP_ERR_INVALID_ARG a pointer is NULL.
     * @retval other SPI error or BUSY timeout.
     */
    esp_err_t (*read_temperature)(void *context, int8_t *out_celsius);
} ink_hal_display_operations_t;

typedef struct {
    const ink_hal_display_operations_t *operations;
    void                               *context;
    ink_hal_display_capabilities_t      capabilities;
} ink_hal_display_device_t;
