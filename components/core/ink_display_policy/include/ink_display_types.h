/* Design: docs/design/display/display.md */
#pragma once

/*
 * Types shared by display_policy, display and the display HAL. Types only, no logic, no ESP-IDF include:
 * every layer depends on this header instead of copying enums between layers.
 */
#include <stdint.h>

/** Intent of a frame.
 * WARN: append only; the app and the bench firmware send these values as numbers. */
typedef enum : uint8_t {
    INK_DISPLAY_INTENT_AUTO, /**< policy picks UI or TEXT from the changed window area */
    INK_DISPLAY_INTENT_TEXT, /**< text page the user will read */
    INK_DISPLAY_INTENT_UI,   /**< cursor, buttons: fast, accepts light ghosting */
    INK_DISPLAY_INTENT_PAGE, /**< whole-panel change */
    INK_DISPLAY_INTENT_COUNT,
} ink_display_intent_t;

/** Kind of cleanup. 0 is invalid, so a zero-initialized struct is rejected.
 * WARN: append only; options and the bench firmware index by these values. */
typedef enum : uint8_t {
    INK_DISPLAY_CLEANUP_INVALID = 0,
    INK_DISPLAY_CLEANUP_STRONG,        /**< whole panel, inverted old plane */
    INK_DISPLAY_CLEANUP_FAST,          /**< pixels with debt or an edge flag; debt ceiling exceeded during an active period */
    INK_DISPLAY_CLEANUP_EDGE_CENTER,   /**< cleanup stage 1: pixels with an edge flag */
    INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR, /**< cleanup stage 1, after edge center: neighbors of flagged pixels */
    INK_DISPLAY_CLEANUP_LIGHT,         /**< cleanup stage 2: pixels with debt >= debt_light */
    INK_DISPLAY_CLEANUP_COUNT,
} ink_display_cleanup_t;

/** What the display does with the panel. 0 is invalid. Policy returns NONE, DRAW or CLEANUP; only the display
 * report uses POWER_OFF.
 * WARN: append only; the bench firmware sends these values as numbers. */
typedef enum : uint8_t {
    INK_DISPLAY_ACTION_INVALID = 0,
    INK_DISPLAY_ACTION_NONE,      /**< nothing to draw */
    INK_DISPLAY_ACTION_DRAW,      /**< draw a frame with the draw preset of `intent` */
    INK_DISPLAY_ACTION_CLEANUP,   /**< cleanup with the cleanup preset of `cleanup` */
    INK_DISPLAY_ACTION_POWER_OFF, /**< analog power off (display report only) */
} ink_display_action_t;

/** Layout of the pixels in a frame. 0 is invalid, so a zero-initialized struct is rejected.
 * WARN: append only; drivers, display and ui compare these values. */
typedef enum : uint8_t {
    INK_DISPLAY_PIXEL_FORMAT_INVALID = 0,
    INK_DISPLAY_PIXEL_FORMAT_K1, /**< 1 bit per pixel (1 = white), row by row, leftmost pixel in the most significant bit */
    /* EXTEND(display): gray levels and color panels add their formats here. */
} ink_display_pixel_format_t;

/** What the app needs to know about a frame; the same for every frame of one panel. */
typedef struct {
    uint16_t                   width; /**< native panel orientation, px */
    uint16_t                   height;
    ink_display_pixel_format_t format;
} ink_display_frame_info_t;

/** Coordinates in native panel orientation, px. */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_display_rect_t;
