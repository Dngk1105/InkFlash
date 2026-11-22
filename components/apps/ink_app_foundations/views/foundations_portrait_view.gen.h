/* Generated from Figma: SU7kpiOJEQ2DaBnUmDPBff node 57:2 (version 2424724397402281751); hash: sha256:0df1c6f026998d4c */
/* Design: docs/design/figma-to-code.md */
#pragma once

/* View foundations_portrait_view: Figma frame foundations_portrait. Do not edit; edit Figma and run tools/figma2ink.py again. */
#include "ink_ui.h"
#include <stdint.h>

#define FOUNDATIONS_PORTRAIT_VIEW_WIDTH  240 /**< canvas width after rotation, px */
#define FOUNDATIONS_PORTRAIT_VIEW_HEIGHT 416 /**< canvas height after rotation, px */

/** Dynamic data of the screen, one field per named layer. */
typedef struct {
    uint8_t unused; /**< the screen has no dynamic layer */
} foundations_portrait_view_data_t;

/** @brief Draw the whole screen. */
void foundations_portrait_view_draw(ink_ui_canvas_t *canvas, const foundations_portrait_view_data_t *data);
