/* Design: docs/design/ui.md */
#include "ink_ui_internal.h"

/* ---- Layout of one text ---- */

#define MAX_LINES 32 /* lines kept per layout; text beyond is treated as cut. NOTE: layout_t lives on the caller's stack */

/* One line: bytes [start, start + length) of the text, without the space or newline that ended it. */
typedef struct {
    const char *start;
    uint16_t    length;
    int16_t     width; /* px, in the selected font */
} line_t;

typedef struct {
    line_t  items[MAX_LINES];
    uint8_t count;
    bool    is_cut; /* text continues after the last kept line */
} layout_t;

/* ---- Leaf functions ---- */
/* LEAF: every prototype in this block gets its definition here, in place of the prototype. */

/* Select `font` in the backend: transparent glyph background, y = top of the line. */
static void select_font(ink_ui_canvas_t *canvas, const ink_ui_font_t *font);

/* Height of one line of the selected font, px: ascent plus descent. */
static int16_t font_line_height(ink_ui_canvas_t *canvas);

/* Break UTF-8 `text` into lines no wider than max_width px in the selected font: break at spaces, a newline forces a
 * break, a word wider than max_width breaks between characters (never inside a UTF-8 sequence), at least one
 * character per line. Stops after MAX_LINES lines and sets is_cut when text is left. */
static void break_lines(ink_ui_canvas_t *canvas, const char *text, int16_t max_width, layout_t *out_layout);

/* Keep at most `limit` lines (0 = no limit); set is_cut when lines are removed. */
static void keep_lines(layout_t *layout, uint8_t limit);

/* Number of lines of `line_height` px with `line_gap` px between them that fit into `height` px, capped by max_lines
 * (0 = no cap) and by MAX_LINES. */
static uint8_t lines_that_fit(int16_t height, int16_t line_height, int8_t line_gap, uint8_t max_lines);

/* Shorten the last line of a cut layout until the line plus the ellipsis fits max_width, and update its width to
 * include the ellipsis. The ellipsis is U+2026 when the selected font has that glyph, three dots otherwise. */
static void reserve_ellipsis(ink_ui_canvas_t *canvas, layout_t *layout, int16_t max_width);

/* Width of the widest line and total height of all lines; {0, 0} for an empty layout. */
static ink_ui_size_t layout_size(const layout_t *layout, int16_t line_height, int8_t line_gap);

/* Draw one line in the current draw color with its top at `top`, placed in [left, left + width) by `align`; append
 * the ellipsis when with_ellipsis is set. */
static void draw_line(ink_ui_canvas_t *canvas, const line_t *line, int16_t left, int16_t width, int16_t top, ink_ui_align_t align,
                      bool with_ellipsis);

/* ---- API ---- */

int16_t ink_ui_text_line_height(ink_ui_canvas_t *canvas, const ink_ui_font_t *font) {
    if (canvas == nullptr || font == nullptr) { return 0; }
    select_font(canvas, font);
    return font_line_height(canvas);
}

ink_ui_size_t ink_ui_text_measure(ink_ui_canvas_t *canvas, const char *text, int16_t max_width, const ink_ui_text_style_t *style) {
    const bool arguments_ok = canvas != nullptr && text != nullptr && style != nullptr && style->font != nullptr;
    if (!arguments_ok || max_width <= 0) { return (ink_ui_size_t){0}; }
    select_font(canvas, style->font);
    layout_t layout;
    break_lines(canvas, text, max_width, &layout);
    keep_lines(&layout, style->max_lines);
    return layout_size(&layout, font_line_height(canvas), style->line_gap);
}

ink_ui_size_t ink_ui_text_draw(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, const char *text, const ink_ui_text_style_t *style,
                               ink_ui_paint_t paint) {
    const bool arguments_ok = ui_ready(canvas) && text != nullptr && style != nullptr && style->font != nullptr;
    if (!arguments_ok || rect.width <= 0 || rect.height <= 0) { return (ink_ui_size_t){0}; }
    select_font(canvas, style->font);
    const int16_t line_height = font_line_height(canvas);
    layout_t      layout;
    break_lines(canvas, text, rect.width, &layout);
    keep_lines(&layout, lines_that_fit(rect.height, line_height, style->line_gap, style->max_lines));
    if (layout.is_cut) { reserve_ellipsis(canvas, &layout, rect.width); }

    ui_apply_solid_paint(canvas, paint);
    int16_t top = rect.y;
    for (uint8_t i = 0; i < layout.count; i++) {
        const bool is_last = i + 1 == layout.count;
        draw_line(canvas, &layout.items[i], rect.x, rect.width, top, style->align, layout.is_cut && is_last);
        top = (int16_t)(top + line_height + style->line_gap);
    }
    return layout_size(&layout, line_height, style->line_gap);
}
