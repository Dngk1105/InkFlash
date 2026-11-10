/* Design: docs/design/ui.md */
#include "ink_ui_internal.h"

#include <string.h>

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

static size_t utf8_char_len(char first) {
    const uint8_t c = (uint8_t)first;
    if ((c & 0x80) == 0) { return 1; }
    if ((c & 0xE0) == 0xC0) { return 2; }
    if ((c & 0xF0) == 0xE0) { return 3; }
    if ((c & 0xF8) == 0xF0) { return 4; }
    return 1;
}

static int16_t measure_span(ink_ui_canvas_t *canvas, const char *text, size_t length) {
    if (length == 0) { return 0; }
    char buf[128];
    if (length >= sizeof(buf)) { length = sizeof(buf) - 1; }
    memcpy(buf, text, length);
    buf[length] = '\0';
    return (int16_t)u8g2_GetUTF8Width(&ui_screen(canvas)->u8g2, buf);
}

/* Select `font` in the backend: transparent glyph background, y = top of the line. */
static void select_font(ink_ui_canvas_t *canvas, const ink_ui_font_t *font) {
    u8g2_t *u8g2 = &ui_screen(canvas)->u8g2;
    u8g2_SetFont(u8g2, font->data);
    u8g2_SetFontRefHeightAll(u8g2);
    u8g2_SetFontMode(u8g2, 1);
    u8g2_SetFontPosTop(u8g2);
}

/* Height of one line of the selected font, px: ascent plus descent. */
static int16_t font_line_height(ink_ui_canvas_t *canvas) {
    u8g2_t       *u8g2   = &ui_screen(canvas)->u8g2;
    const int16_t height = (int16_t)(u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2));
    return height > 0 ? height : 0;
}

/* Break UTF-8 `text` into lines no wider than max_width px in the selected font: break at spaces, a newline forces a
 * break, a word wider than max_width breaks between characters (never inside a UTF-8 sequence), at least one
 * character per line. Stops after MAX_LINES lines and sets is_cut when text is left. */
static void break_lines(ink_ui_canvas_t *canvas, const char *text, int16_t max_width, layout_t *out_layout) {
    out_layout->count  = 0;
    out_layout->is_cut = false;
    if (text == nullptr || *text == '\0' || max_width <= 0) { return; }

    const char *p = text;
    while (*p != '\0' && out_layout->count < MAX_LINES) {
        if (*p == '\n') {
            out_layout->items[out_layout->count++] = (line_t){.start = p, .length = 0, .width = 0};
            p++;
            continue;
        }

        const char *line_start = p;
        const char *last_end   = nullptr;
        int16_t     last_width = 0;
        const char *next_line  = nullptr;
        const char *curr       = p;

        while (*curr != '\0' && *curr != '\n') {
            const char *word_start = curr;
            while (*word_start == ' ') { word_start++; }
            if (*word_start == '\0' || *word_start == '\n') {
                curr = word_start;
                break;
            }

            const char *word_end = word_start;
            while (*word_end != '\0' && *word_end != ' ' && *word_end != '\n') { word_end += utf8_char_len(*word_end); }

            const size_t  cand_len = (size_t)(word_end - line_start);
            const int16_t cand_w   = measure_span(canvas, line_start, cand_len);

            if (cand_w <= max_width) {
                last_end   = word_end;
                last_width = cand_w;
                curr       = word_end;
            } else {
                if (last_end != nullptr) {
                    next_line = word_start;
                } else {
                    const char *cp      = word_start;
                    const char *prev_cp = cp;
                    cp += utf8_char_len(*cp);
                    prev_cp   = cp;
                    int16_t w = measure_span(canvas, word_start, (size_t)(cp - word_start));

                    while (*cp != '\0' && *cp != ' ' && *cp != '\n') {
                        const size_t  char_len = utf8_char_len(*cp);
                        const size_t  new_len  = (size_t)(cp + char_len - word_start);
                        const int16_t next_w   = measure_span(canvas, word_start, new_len);
                        if (next_w > max_width) { break; }
                        prev_cp = cp + char_len;
                        w       = next_w;
                        cp += char_len;
                    }
                    last_end   = prev_cp;
                    last_width = w;
                    next_line  = prev_cp;
                }
                break;
            }
        }

        if (last_end == nullptr) {
            last_end   = line_start;
            last_width = 0;
        }

        out_layout->items[out_layout->count++] = (line_t){
            .start  = line_start,
            .length = (uint16_t)(last_end - line_start),
            .width  = last_width,
        };

        if (next_line != nullptr) {
            p = next_line;
        } else if (*curr == '\n') {
            p = curr + 1;
        } else {
            p = curr;
        }
    }

    if (*p != '\0') { out_layout->is_cut = true; }
}

/* Keep at most `limit` lines (0 = no limit); set is_cut when lines are removed. */
static void keep_lines(layout_t *layout, uint8_t limit) {
    if (limit > 0 && layout->count > limit) {
        layout->count  = limit;
        layout->is_cut = true;
    }
}

/* Number of lines of `line_height` px with `line_gap` px between them that fit into `height` px, capped by max_lines
 * (0 = no cap) and by MAX_LINES. */
static uint8_t lines_that_fit(int16_t height, int16_t line_height, int8_t line_gap, uint8_t max_lines) {
    if (height <= 0 || line_height <= 0) { return 0; }
    uint8_t cap = MAX_LINES;
    if (max_lines > 0 && max_lines < cap) { cap = max_lines; }
    uint8_t count  = 0;
    int32_t needed = 0;
    for (uint8_t i = 1; i <= cap; i++) {
        if (i == 1) {
            needed = line_height;
        } else {
            needed += (int32_t)line_height + line_gap;
        }
        if (needed <= height) {
            count = i;
        } else {
            break;
        }
    }
    return count;
}

/* Shorten the last line of a cut layout until the line plus the ellipsis fits max_width, and update its width to
 * include the ellipsis. The ellipsis is U+2026 when the selected font has that glyph, three dots otherwise. */
static void reserve_ellipsis(ink_ui_canvas_t *canvas, layout_t *layout, int16_t max_width) {
    if (layout == nullptr || layout->count == 0) { return; }
    line_t       *last     = &layout->items[layout->count - 1];
    u8g2_t       *u8g2     = &ui_screen(canvas)->u8g2;
    const char   *ellipsis = u8g2_IsGlyph(u8g2, 0x2026) ? "\xE2\x80\xA6" : "...";
    const int16_t el_w     = (int16_t)u8g2_GetUTF8Width(u8g2, ellipsis);

    while (last->length > 0) {
        const int16_t text_w = measure_span(canvas, last->start, last->length);
        if (text_w + el_w <= max_width) {
            last->width = (int16_t)(text_w + el_w);
            return;
        }
        size_t prev_len = last->length - 1;
        while (prev_len > 0 && ((uint8_t)last->start[prev_len] & 0xC0) == 0x80) { prev_len--; }
        last->length = (uint16_t)prev_len;
    }
    last->width = el_w;
}

/* Width of the widest line and total height of all lines; {0, 0} for an empty layout. */
static ink_ui_size_t layout_size(const layout_t *layout, int16_t line_height, int8_t line_gap) {
    if (layout == nullptr || layout->count == 0 || line_height <= 0) { return (ink_ui_size_t){0, 0}; }
    int16_t max_w = 0;
    for (uint8_t i = 0; i < layout->count; i++) {
        if (layout->items[i].width > max_w) { max_w = layout->items[i].width; }
    }
    const int32_t total_h = (int32_t)layout->count * line_height + (int32_t)(layout->count - 1) * line_gap;
    return (ink_ui_size_t){
        .width  = max_w,
        .height = (int16_t)(total_h > 0 ? total_h : 0),
    };
}

/* Draw one line in the current draw color with its top at `top`, placed in [left, left + width) by `align`; append
 * the ellipsis when with_ellipsis is set. */
static void draw_line(ink_ui_canvas_t *canvas, const line_t *line, int16_t left, int16_t width, int16_t top, ink_ui_align_t align,
                      bool with_ellipsis) {
    u8g2_t *u8g2 = &ui_screen(canvas)->u8g2;
    int16_t x    = left;
    if (align == INK_UI_ALIGN_CENTER) {
        x = (int16_t)(left + (width - line->width) / 2);
    } else if (align == INK_UI_ALIGN_END) {
        x = (int16_t)(left + width - line->width);
    }
    if (x < 0) { x = 0; }
    if (top < 0) { top = 0; }

    const u8g2_uint_t old_x0 = u8g2->clip_x0;
    const u8g2_uint_t old_x1 = u8g2->clip_x1;
    const u8g2_uint_t old_y0 = u8g2->clip_y0;
    const u8g2_uint_t old_y1 = u8g2->clip_y1;

    const int16_t lh  = font_line_height(canvas);
    int16_t       cx0 = left > (int16_t)old_x0 ? left : (int16_t)old_x0;
    int16_t       cy0 = top > (int16_t)old_y0 ? top : (int16_t)old_y0;
    int16_t       cx1 = (int16_t)(left + width);
    if (cx1 > (int16_t)old_x1) { cx1 = (int16_t)old_x1; }
    int16_t cy1 = (int16_t)(top + lh);
    if (cy1 > (int16_t)old_y1) { cy1 = (int16_t)old_y1; }

    if (cx1 > cx0 && cy1 > cy0) { u8g2_SetClipWindow(u8g2, (u8g2_uint_t)cx0, (u8g2_uint_t)cy0, (u8g2_uint_t)cx1, (u8g2_uint_t)cy1); }

    char   buf[128];
    size_t len = line->length;
    if (with_ellipsis) {
        const char  *ellipsis = u8g2_IsGlyph(u8g2, 0x2026) ? "\xE2\x80\xA6" : "...";
        const size_t elen     = strlen(ellipsis);
        if (len + elen >= sizeof(buf)) { len = sizeof(buf) - 1 - elen; }
        memcpy(buf, line->start, len);
        memcpy(buf + len, ellipsis, elen);
        buf[len + elen] = '\0';
    } else {
        if (len >= sizeof(buf)) { len = sizeof(buf) - 1; }
        memcpy(buf, line->start, len);
        buf[len] = '\0';
    }

    u8g2_DrawUTF8(u8g2, (u8g2_uint_t)x, (u8g2_uint_t)top, buf);
    u8g2_SetClipWindow(u8g2, old_x0, old_y0, old_x1, old_y1);
}

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
