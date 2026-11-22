/* Generated from Figma: SU7kpiOJEQ2DaBnUmDPBff node 57:100 (version 2424762519534052102); hash: sha256:0616f214a5484f84 */
/* Design: docs/design/figma-to-code.md */
#pragma once

/* View foundations_landscape_view: Figma frame foundations_landscape. Do not edit; edit Figma and run tools/figma2ink.py again. */
#include "ink_ui.h"
#include <stdint.h>

#define FOUNDATIONS_LANDSCAPE_VIEW_WIDTH  416 /**< canvas width after rotation, px */
#define FOUNDATIONS_LANDSCAPE_VIEW_HEIGHT 240 /**< canvas height after rotation, px */

/** Dynamic data of the screen, one field per named layer. */
typedef struct {
    uint8_t unused; /**< the screen has no dynamic layer */
} foundations_landscape_view_data_t;

/** @brief Draw the whole screen. */
void foundations_landscape_view_draw(ink_ui_canvas_t *canvas, const foundations_landscape_view_data_t *data);
