/* Design: docs/design/ui.md */
#pragma once

/* Shared by the .c files of ink_ui; other components cannot include this header. */
#include "ink_u8g2.h"
#include "ink_ui.h"

/* ---- Internal helpers ---- */

/* Drawing backend stored in canvas->priv.screen; canvas must not be NULL. */
ink_u8g2_screen_t *ui_screen(ink_ui_canvas_t *canvas);

/* True when draw calls may write pixels: canvas is not NULL and a frame is bound. */
bool ui_ready(const ink_ui_canvas_t *canvas);

/* Brightness of an RGB color using standard coefficients: 0 (black) to 255 (white). */
uint8_t ui_brightness(ink_ui_color_t color);

/* Select the U8g2 draw color for strokes, lines, text and bitmaps: XOR mode -> 2 (invert); brightness below 50 % ->
 * 0 (black, clears the bit); otherwise 1 (white). */
void ui_apply_solid_paint(ink_ui_canvas_t *canvas, ink_ui_paint_t paint);
