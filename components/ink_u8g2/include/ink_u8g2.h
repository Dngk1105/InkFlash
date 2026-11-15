/* Design: docs/design/ui.md */
#pragma once

/*
 * Wrapper of the U8g2 library (third_party/u8g2). Sets U8g2 up as a virtual screen that draws straight into a K1
 * frame: row by row, leftmost pixel in the most significant bit, 1 = white. No display driver of U8g2 is used and
 * nothing is ever sent to a bus. Only ui includes this header.
 */
#include "u8g2.h"
#include <stdint.h>

/* ---- Types ---- */

/** One virtual screen.
 * WARN: do not copy or move after init; U8g2 keeps a pointer to `info`. */
typedef struct {
    u8g2_t              u8g2; /**< pass to u8g2_* calls */
    u8x8_display_info_t info; /**< display info referenced by U8g2 */
} ink_u8g2_screen_t;

/* ---- API ---- */

/**
 * @brief Set up a virtual screen of `width` × `height` px with no frame bound yet.
 *
 * Bind a frame with u8g2_SetBufferPtr() before drawing. Draw color 0 clears a bit (black), 1 sets it (white),
 * 2 inverts it.
 * @par Called from
 * Any task; the caller owns `screen`.
 * @param width, height Native panel orientation, px.
 * @param quarter_turns Clockwise rotation in steps of 90°, 0–3.
 * @retval true Ready.
 * @retval false screen is NULL, a size is 0, quarter_turns > 3, or the frame is larger than U8g2 can address
 *         (65 535 bytes, 2 040 px per side).
 */
bool ink_u8g2_screen_init(ink_u8g2_screen_t *screen, uint16_t width, uint16_t height, uint8_t quarter_turns);
