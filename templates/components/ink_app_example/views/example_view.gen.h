/* Generated from Figma: none (hand-written sample of the view generator output); hash: none */
/* Design: docs/design/figma-to-code.md */
#pragma once

/*
 * View of the example screen: a black header with {title}, a list *rows of {label}, the hint ?empty_hint and a static
 * gray band and footer. Parts: part:header, part:body.
 */
#include "ink_ui.h"
#include <stdint.h>

#define EXAMPLE_VIEW_ROWS_MAX 4 /**< rows drawn in Figma for *rows */

/** One row of *rows. */
typedef struct {
    const char *label; /**< {label} */
} example_view_rows_row_t;

/** Dynamic data of the screen, one field per named layer. */
typedef struct {
    const char             *title;                       /**< {title} */
    example_view_rows_row_t rows[EXAMPLE_VIEW_ROWS_MAX]; /**< *rows */
    uint8_t                 rows_count;                  /**< rows used in `rows` */
    bool                    show_empty_hint;             /**< ?empty_hint */
} example_view_data_t;

/** @brief Draw the whole screen. */
void example_view_draw(ink_ui_canvas_t *canvas, const example_view_data_t *data);

/** @brief Draw part:header only (its own background included). */
void example_view_draw_header(ink_ui_canvas_t *canvas, const example_view_data_t *data);

/** @brief Draw part:body only (its own background included). */
void example_view_draw_body(ink_ui_canvas_t *canvas, const example_view_data_t *data);
