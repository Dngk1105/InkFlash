/* Generated from Figma: SU7kpiOJEQ2DaBnUmDPBff node 69:503 (version 2424764074327803648); hash: sha256:9eebb3d718c47541 */
/* Design: docs/design/figma-to-code.md */
#pragma once

/* View foundations_2_landscape_view: Figma frame foundations_2_landscape. Do not edit; edit Figma and run tools/figma2ink.py again. */
#include "ink_ui.h"
#include <stdint.h>

#define FOUNDATIONS_2_LANDSCAPE_VIEW_WIDTH  416 /**< canvas width after rotation, px */
#define FOUNDATIONS_2_LANDSCAPE_VIEW_HEIGHT 240 /**< canvas height after rotation, px */

/** Dynamic data of the screen, one field per named layer. */
typedef struct {
    uint8_t unused; /**< the screen has no dynamic layer */
} foundations_2_landscape_view_data_t;

/** @brief Draw the whole screen. */
void foundations_2_landscape_view_draw(ink_ui_canvas_t *canvas, const foundations_2_landscape_view_data_t *data);
