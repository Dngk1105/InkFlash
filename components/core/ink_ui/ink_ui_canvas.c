/* Design: docs/design/ui.md */
#include "ink_ui_internal.h"

static_assert(sizeof(ink_u8g2_screen_t) <= INK_UI_CANVAS_SCREEN_BYTES, "raise INK_UI_CANVAS_SCREEN_BYTES");

/* ---- Leaf functions ---- */
/* LEAF: every prototype in this block gets its definition here, in place of the prototype. */

/* Common area of two rects; empty (width or height 0) when they do not overlap. */
static ink_ui_rect_t rect_intersect(ink_ui_rect_t a, ink_ui_rect_t b);

/* True when width or height is <= 0. */
static bool rect_is_empty(ink_ui_rect_t rect);

/* Set the U8g2 clip window to `area`; `area` lies inside the canvas bounds and is not empty. */
static void set_backend_clip(ink_ui_canvas_t *canvas, ink_ui_rect_t area);

/* True when the paint is COPY mode with a brightness strictly between black (0) and white (255). */
static bool paint_is_dithered(ink_ui_paint_t paint);

/* Fill `rect` (clipped to the canvas bounds) with a 4 × 4 ordered dither of `brightness`: the share of white pixels
 * grows with brightness, the pattern is anchored at the canvas origin so neighboring fills line up. */
static void fill_dither(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t brightness);

/* Fill `rect` (clipped to the canvas bounds) in the current draw color; rounded corners when radius > 0. The radius
 * is cut to what the rect can hold. */
static void fill_solid(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t radius);

/* Draw a 1 px outline on the edge of `ring` in the current draw color; rounded corners when radius > 0. No pixel
 * is drawn twice, so XOR leaves no gap. */
static void stroke_ring(ink_ui_canvas_t *canvas, ink_ui_rect_t ring, uint8_t radius);

/* Draw a line of `thickness` px centered on the segment (x0, y0)-(x1, y1) in the current draw color. */
static void draw_thick_line(ink_ui_canvas_t *canvas, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness);

/* True when bitmap is not NULL, has data, a non-zero size and format MASK1. */
static bool bitmap_is_valid(const ink_ui_bitmap_t *bitmap);

/* Draw every 1 bit of a MASK1 bitmap in the current draw color with its top left corner at (x, y); 0 bits and
 * pixels outside the canvas are skipped. */
static void draw_mask(ink_ui_canvas_t *canvas, int16_t x, int16_t y, const ink_ui_bitmap_t *bitmap);

/* ---- Shared with ink_ui_text.c ---- */

ink_u8g2_screen_t *ui_screen(ink_ui_canvas_t *canvas) {
    return (ink_u8g2_screen_t *)canvas->priv.screen;
}

bool ui_ready(const ink_ui_canvas_t *canvas) {
    return canvas != nullptr && canvas->priv.frame != nullptr;
}

/* LEAF: ui_brightness() and ui_apply_solid_paint() (contracts in ink_ui_internal.h) are defined here. */

/* ---- Canvas ---- */

ink_ui_error_t ink_ui_canvas_init(ink_ui_canvas_t *canvas, const ink_display_frame_info_t *info, ink_ui_rotation_t rotation) {
    if (canvas == nullptr || info == nullptr) { return INK_UI_ERROR_ARGUMENT; }
    if (info->width == 0 || info->height == 0 || rotation > INK_UI_ROTATION_270) { return INK_UI_ERROR_ARGUMENT; }
    /* EXTEND(ui): gray and color pixel formats need their own drawing backend behind this API. */
    if (info->format != INK_DISPLAY_PIXEL_FORMAT_K1) { return INK_UI_ERROR_FORMAT; }
    canvas->priv.info     = *info;
    canvas->priv.rotation = rotation;
    canvas->priv.frame    = nullptr;
    if (!ink_u8g2_screen_init(ui_screen(canvas), info->width, info->height, (uint8_t)rotation)) { return INK_UI_ERROR_FORMAT; }
    return INK_UI_OK;
}

void ink_ui_canvas_bind(ink_ui_canvas_t *canvas, uint8_t *frame) {
    if (canvas == nullptr) { return; }
    canvas->priv.frame                   = frame;
    ui_screen(canvas)->u8g2.tile_buf_ptr = frame;
}

ink_ui_rect_t ink_ui_canvas_bounds(const ink_ui_canvas_t *canvas) {
    if (canvas == nullptr) { return (ink_ui_rect_t){0}; }
    const bool sideways = canvas->priv.rotation == INK_UI_ROTATION_90 || canvas->priv.rotation == INK_UI_ROTATION_270;
    return (ink_ui_rect_t){
        .width  = (int16_t)(sideways ? canvas->priv.info.height : canvas->priv.info.width),
        .height = (int16_t)(sideways ? canvas->priv.info.width : canvas->priv.info.height),
    };
}

void ink_ui_canvas_set_clip(ink_ui_canvas_t *canvas, ink_ui_rect_t clip) {
    if (canvas == nullptr) { return; }
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    ink_ui_rect_t       area   = rect_intersect(clip, bounds);
    if (rect_is_empty(area)) { area = bounds; }
    set_backend_clip(canvas, area);
}

uint8_t *ink_ui_canvas_frame(const ink_ui_canvas_t *canvas, ink_display_frame_info_t *out_info) {
    if (canvas == nullptr) { return nullptr; }
    if (out_info != nullptr) { *out_info = canvas->priv.info; }
    return canvas->priv.frame;
}

/* ---- Primitives ---- */

void ink_ui_fill(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t radius, ink_ui_paint_t paint) {
    if (!ui_ready(canvas) || rect_is_empty(rect)) { return; }
    if (paint_is_dithered(paint)) {
        /* TODO(ui): rounded corners for dithered fills. */
        fill_dither(canvas, rect, ui_brightness(paint.color));
        return;
    }
    ui_apply_solid_paint(canvas, paint);
    fill_solid(canvas, rect, radius);
}

void ink_ui_stroke(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t thickness, uint8_t radius, ink_ui_paint_t paint) {
    if (!ui_ready(canvas) || rect_is_empty(rect)) { return; }
    ui_apply_solid_paint(canvas, paint);
    for (uint8_t depth = 0; depth < thickness; depth++) {
        const ink_ui_rect_t ring = ink_ui_rect_inset(rect, depth, depth);
        if (rect_is_empty(ring)) { break; }
        stroke_ring(canvas, ring, radius > depth ? (uint8_t)(radius - depth) : 0);
    }
}

void ink_ui_line(ink_ui_canvas_t *canvas, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness, ink_ui_paint_t paint) {
    if (!ui_ready(canvas) || thickness == 0) { return; }
    ui_apply_solid_paint(canvas, paint);
    draw_thick_line(canvas, x0, y0, x1, y1, thickness);
}

void ink_ui_bitmap(ink_ui_canvas_t *canvas, int16_t x, int16_t y, const ink_ui_bitmap_t *bitmap, ink_ui_paint_t paint) {
    if (!ui_ready(canvas) || !bitmap_is_valid(bitmap)) { return; }
    ui_apply_solid_paint(canvas, paint);
    draw_mask(canvas, x, y, bitmap);
}
