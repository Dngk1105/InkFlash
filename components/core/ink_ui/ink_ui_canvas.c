/* Design: docs/design/ui.md */
#include "ink_ui_internal.h"

static_assert(sizeof(ink_u8g2_screen_t) <= INK_UI_CANVAS_SCREEN_BYTES, "raise INK_UI_CANVAS_SCREEN_BYTES");

/* ---- Leaf functions ---- */

/* Common area of two rects; empty (width or height 0) when they do not overlap. */
static ink_ui_rect_t rect_intersect(ink_ui_rect_t a, ink_ui_rect_t b) {
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0) { return (ink_ui_rect_t){0}; }
    const int16_t x0   = a.x > b.x ? a.x : b.x;
    const int16_t y0   = a.y > b.y ? a.y : b.y;
    const int16_t x1_a = (int16_t)(a.x + a.width);
    const int16_t x1_b = (int16_t)(b.x + b.width);
    const int16_t x1   = x1_a < x1_b ? x1_a : x1_b;
    const int16_t y1_a = (int16_t)(a.y + a.height);
    const int16_t y1_b = (int16_t)(b.y + b.height);
    const int16_t y1   = y1_a < y1_b ? y1_a : y1_b;
    if (x1 <= x0 || y1 <= y0) { return (ink_ui_rect_t){0}; }
    return (ink_ui_rect_t){
        .x      = x0,
        .y      = y0,
        .width  = (int16_t)(x1 - x0),
        .height = (int16_t)(y1 - y0),
    };
}

/* True when width or height is <= 0. */
static bool rect_is_empty(ink_ui_rect_t rect) {
    return rect.width <= 0 || rect.height <= 0;
}

/* Set the U8g2 clip window to `area`; `area` lies inside the canvas bounds and is not empty. */
static void set_backend_clip(ink_ui_canvas_t *canvas, ink_ui_rect_t area) {
    u8g2_SetClipWindow(&ui_screen(canvas)->u8g2, (u8g2_uint_t)area.x, (u8g2_uint_t)area.y, (u8g2_uint_t)(area.x + area.width),
                       (u8g2_uint_t)(area.y + area.height));
}

/* True when the paint is COPY mode with a brightness strictly between black (0) and white (255). */
static bool paint_is_dithered(ink_ui_paint_t paint) {
    if (paint.mode != INK_UI_PAINT_MODE_COPY) { return false; }
    const uint8_t b = ui_brightness(paint.color);
    return b > 0 && b < 255;
}

/* Fill `rect` (clipped to the canvas bounds) with a 4 × 4 clustered-dot dither of `brightness`: white pixels grow as
 * one cluster from the middle of each cell, the pattern is anchored at the canvas origin so neighboring fills line up.
 * NOTE: clustered, not Bayer: on BB0 a checkerboard-like dither made black on the same rows fade, because many source
 * lines switch level at every row (refresh.md, 2026-11-12). */
static void fill_dither(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t brightness) {
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    const ink_ui_rect_t area   = rect_intersect(rect, bounds);
    if (rect_is_empty(area)) { return; }

    static const uint8_t s_cluster4x4[4][4] = {
        {12, 5,  6, 13},
        {4,  0,  1, 7 },
        {11, 3,  2, 8 },
        {15, 10, 9, 14},
    };

    u8g2_t *u8g2 = &ui_screen(canvas)->u8g2;
    for (int16_t y = area.y; y < area.y + area.height; y++) {
        for (int16_t x = area.x; x < area.x + area.width; x++) {
            const uint8_t threshold = (uint8_t)(s_cluster4x4[y & 3][x & 3] * 16 + 8);
            const uint8_t color     = brightness > threshold ? 1 : 0;
            u8g2_SetDrawColor(u8g2, color);
            u8g2_DrawPixel(u8g2, (u8g2_uint_t)x, (u8g2_uint_t)y);
        }
    }
}

/* Fill `rect` (clipped to the canvas bounds) in the current draw color; rounded corners when radius > 0. The radius
 * is cut to what the rect can hold. */
static void fill_solid(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t radius) {
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    const ink_ui_rect_t area   = rect_intersect(rect, bounds);
    if (rect_is_empty(area)) { return; }

    uint8_t r = radius;
    if (r > 0) {
        if (area.width < 2 || area.height < 2) {
            r = 0;
        } else {
            const uint8_t max_rx = (uint8_t)(area.width / 2 - 1);
            const uint8_t max_ry = (uint8_t)(area.height / 2 - 1);
            const uint8_t max_r  = max_rx < max_ry ? max_rx : max_ry;
            if (r > max_r) { r = max_r; }
        }
    }

    if (r > 0) {
        u8g2_DrawRBox(&ui_screen(canvas)->u8g2, (u8g2_uint_t)area.x, (u8g2_uint_t)area.y, (u8g2_uint_t)area.width, (u8g2_uint_t)area.height,
                      r);
    } else {
        u8g2_DrawBox(&ui_screen(canvas)->u8g2, (u8g2_uint_t)area.x, (u8g2_uint_t)area.y, (u8g2_uint_t)area.width, (u8g2_uint_t)area.height);
    }
}

/* Draw one pixel in the current draw color; ignored when outside bounds. */
static void draw_canvas_pixel(ink_ui_canvas_t *canvas, int16_t x, int16_t y) {
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    if (x < bounds.x || x >= bounds.x + bounds.width || y < bounds.y || y >= bounds.y + bounds.height) { return; }
    u8g2_DrawPixel(&ui_screen(canvas)->u8g2, (u8g2_uint_t)x, (u8g2_uint_t)y);
}

/* Draw a horizontal line clipped to bounds in the current draw color. */
static void draw_canvas_hline(ink_ui_canvas_t *canvas, int16_t x0, int16_t x1, int16_t y) {
    if (x1 < x0) { return; }
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    if (y < bounds.y || y >= bounds.y + bounds.height) { return; }
    if (x0 < bounds.x) { x0 = bounds.x; }
    if (x1 >= bounds.x + bounds.width) { x1 = (int16_t)(bounds.x + bounds.width - 1); }
    if (x0 <= x1) { u8g2_DrawHLine(&ui_screen(canvas)->u8g2, (u8g2_uint_t)x0, (u8g2_uint_t)y, (u8g2_uint_t)(x1 - x0 + 1)); }
}

/* Draw a vertical line clipped to bounds in the current draw color. */
static void draw_canvas_vline(ink_ui_canvas_t *canvas, int16_t x, int16_t y0, int16_t y1) {
    if (y1 < y0) { return; }
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    if (x < bounds.x || x >= bounds.x + bounds.width) { return; }
    if (y0 < bounds.y) { y0 = bounds.y; }
    if (y1 >= bounds.y + bounds.height) { y1 = (int16_t)(bounds.y + bounds.height - 1); }
    if (y0 <= y1) { u8g2_DrawVLine(&ui_screen(canvas)->u8g2, (u8g2_uint_t)x, (u8g2_uint_t)y0, (u8g2_uint_t)(y1 - y0 + 1)); }
}

/* Draw the 8 symmetric corner pixels of rounded corners for stroke_ring(). */
static void draw_corner_pixels(ink_ui_canvas_t *canvas, int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1, int16_t dx, int16_t dy) {
    draw_canvas_pixel(canvas, (int16_t)(cx1 + dx), (int16_t)(cy0 - dy));
    draw_canvas_pixel(canvas, (int16_t)(cx0 - dx), (int16_t)(cy0 - dy));
    draw_canvas_pixel(canvas, (int16_t)(cx1 + dx), (int16_t)(cy1 + dy));
    draw_canvas_pixel(canvas, (int16_t)(cx0 - dx), (int16_t)(cy1 + dy));
    if (dx != dy) {
        draw_canvas_pixel(canvas, (int16_t)(cx1 + dy), (int16_t)(cy0 - dx));
        draw_canvas_pixel(canvas, (int16_t)(cx0 - dy), (int16_t)(cy0 - dx));
        draw_canvas_pixel(canvas, (int16_t)(cx1 + dy), (int16_t)(cy1 + dx));
        draw_canvas_pixel(canvas, (int16_t)(cx0 - dy), (int16_t)(cy1 + dx));
    }
}

/* Draw a 1 px outline on the edge of `ring` in the current draw color; rounded corners when radius > 0. No pixel
 * is drawn twice, so XOR leaves no gap. */
static void stroke_ring(ink_ui_canvas_t *canvas, ink_ui_rect_t ring, uint8_t radius) {
    if (rect_is_empty(ring)) { return; }
    if (ring.width == 1) {
        draw_canvas_vline(canvas, ring.x, ring.y, (int16_t)(ring.y + ring.height - 1));
        return;
    }
    if (ring.height == 1) {
        draw_canvas_hline(canvas, ring.x, (int16_t)(ring.x + ring.width - 1), ring.y);
        return;
    }

    uint8_t r = radius;
    if (r > 0) {
        const uint8_t max_rx = (uint8_t)(ring.width / 2 - 1);
        const uint8_t max_ry = (uint8_t)(ring.height / 2 - 1);
        const uint8_t max_r  = max_rx < max_ry ? max_rx : max_ry;
        if (r > max_r) { r = max_r; }
    }

    if (r == 0) {
        draw_canvas_hline(canvas, ring.x, (int16_t)(ring.x + ring.width - 1), ring.y);
        draw_canvas_hline(canvas, ring.x, (int16_t)(ring.x + ring.width - 1), (int16_t)(ring.y + ring.height - 1));
        if (ring.height > 2) {
            draw_canvas_vline(canvas, ring.x, (int16_t)(ring.y + 1), (int16_t)(ring.y + ring.height - 2));
            draw_canvas_vline(canvas, (int16_t)(ring.x + ring.width - 1), (int16_t)(ring.y + 1), (int16_t)(ring.y + ring.height - 2));
        }
        return;
    }

    const int16_t cx0 = (int16_t)(ring.x + r);
    const int16_t cy0 = (int16_t)(ring.y + r);
    const int16_t cx1 = (int16_t)(ring.x + ring.width - 1 - r);
    const int16_t cy1 = (int16_t)(ring.y + ring.height - 1 - r);

    draw_canvas_hline(canvas, (int16_t)(cx0 + 1), (int16_t)(cx1 - 1), ring.y);
    draw_canvas_hline(canvas, (int16_t)(cx0 + 1), (int16_t)(cx1 - 1), (int16_t)(ring.y + ring.height - 1));
    draw_canvas_vline(canvas, ring.x, (int16_t)(cy0 + 1), (int16_t)(cy1 - 1));
    draw_canvas_vline(canvas, (int16_t)(ring.x + ring.width - 1), (int16_t)(cy0 + 1), (int16_t)(cy1 - 1));

    int16_t f     = (int16_t)(1 - (int16_t)r);
    int16_t ddF_x = 1;
    int16_t ddF_y = (int16_t)(-2 * (int16_t)r);
    int16_t dx    = 0;
    int16_t dy    = (int16_t)r;

    draw_corner_pixels(canvas, cx0, cy0, cx1, cy1, dx, dy);
    while (dx < dy) {
        if (f >= 0) {
            dy--;
            ddF_y = (int16_t)(ddF_y + 2);
            f     = (int16_t)(f + ddF_y);
        }
        dx++;
        ddF_x = (int16_t)(ddF_x + 2);
        f     = (int16_t)(f + ddF_x);
        draw_corner_pixels(canvas, cx0, cy0, cx1, cy1, dx, dy);
    }
}

/* Clip segment (x0, y0)-(x1, y1) to canvas bounds using Cohen-Sutherland and draw with U8g2. */
static void draw_clipped_line(ink_ui_canvas_t *canvas, int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(canvas);
    if (bounds.width <= 0 || bounds.height <= 0) { return; }
    const int16_t xmin = bounds.x;
    const int16_t xmax = (int16_t)(bounds.x + bounds.width - 1);
    const int16_t ymin = bounds.y;
    const int16_t ymax = (int16_t)(bounds.y + bounds.height - 1);

    int32_t px0 = x0, py0 = y0, px1 = x1, py1 = y1;
    while (true) {
        const uint8_t c0 = (uint8_t)((px0 < xmin ? 1 : 0) | (px0 > xmax ? 2 : 0) | (py0 < ymin ? 4 : 0) | (py0 > ymax ? 8 : 0));
        const uint8_t c1 = (uint8_t)((px1 < xmin ? 1 : 0) | (px1 > xmax ? 2 : 0) | (py1 < ymin ? 4 : 0) | (py1 > ymax ? 8 : 0));
        if ((c0 | c1) == 0) {
            u8g2_DrawLine(&ui_screen(canvas)->u8g2, (u8g2_uint_t)px0, (u8g2_uint_t)py0, (u8g2_uint_t)px1, (u8g2_uint_t)py1);
            return;
        }
        if ((c0 & c1) != 0) { return; }
        const uint8_t c  = c0 != 0 ? c0 : c1;
        int32_t       nx = 0, ny = 0;
        if ((c & 8) != 0) {
            nx = px0 + (px1 - px0) * (ymax - py0) / (py1 - py0);
            ny = ymax;
        } else if ((c & 4) != 0) {
            nx = px0 + (px1 - px0) * (ymin - py0) / (py1 - py0);
            ny = ymin;
        } else if ((c & 2) != 0) {
            ny = py0 + (py1 - py0) * (xmax - px0) / (px1 - px0);
            nx = xmax;
        } else if ((c & 1) != 0) {
            ny = py0 + (py1 - py0) * (xmin - px0) / (px1 - px0);
            nx = xmin;
        }
        if (c == c0) {
            px0 = nx;
            py0 = ny;
        } else {
            px1 = nx;
            py1 = ny;
        }
    }
}

/* Draw a line of `thickness` px centered on the segment (x0, y0)-(x1, y1) in the current draw color. */
static void draw_thick_line(ink_ui_canvas_t *canvas, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness) {
    if (thickness == 0) { return; }
    const int16_t dx      = (int16_t)(x1 >= x0 ? x1 - x0 : x0 - x1);
    const int16_t dy      = (int16_t)(y1 >= y0 ? y1 - y0 : y0 - y1);
    const int16_t start_d = (int16_t)(-(thickness / 2));
    for (uint8_t i = 0; i < thickness; i++) {
        const int16_t d = (int16_t)(start_d + i);
        if (dx >= dy) {
            draw_clipped_line(canvas, x0, (int16_t)(y0 + d), x1, (int16_t)(y1 + d));
        } else {
            draw_clipped_line(canvas, (int16_t)(x0 + d), y0, (int16_t)(x1 + d), y1);
        }
    }
}

/* True when bitmap is not NULL, has data, a non-zero size and format MASK1. */
static bool bitmap_is_valid(const ink_ui_bitmap_t *bitmap) {
    return bitmap != nullptr
        && bitmap->data != nullptr
        && bitmap->width > 0
        && bitmap->height > 0
        && bitmap->format == INK_UI_BITMAP_FORMAT_MASK1;
}

/* Draw every 1 bit of a MASK1 bitmap in the current draw color with its top left corner at (x, y); 0 bits and
 * pixels outside the canvas are skipped. */
static void draw_mask(ink_ui_canvas_t *canvas, int16_t x, int16_t y, const ink_ui_bitmap_t *bitmap) {
    const ink_ui_rect_t bounds     = ink_ui_canvas_bounds(canvas);
    const size_t        row_stride = ((size_t)bitmap->width + 7) / 8;
    for (uint16_t r = 0; r < bitmap->height; r++) {
        const int16_t py = (int16_t)(y + r);
        if (py < bounds.y || py >= bounds.y + bounds.height) { continue; }
        const uint8_t *row_data = bitmap->data + (size_t)r * row_stride;
        for (uint16_t c = 0; c < bitmap->width; c++) {
            const int16_t px = (int16_t)(x + c);
            if (px < bounds.x || px >= bounds.x + bounds.width) { continue; }
            const uint8_t byte = row_data[c / 8];
            if ((byte & (0x80 >> (c % 8))) != 0) { u8g2_DrawPixel(&ui_screen(canvas)->u8g2, (u8g2_uint_t)px, (u8g2_uint_t)py); }
        }
    }
}

/* ---- Shared with ink_ui_text.c ---- */

ink_u8g2_screen_t *ui_screen(ink_ui_canvas_t *canvas) {
    return (ink_u8g2_screen_t *)canvas->priv.screen;
}

bool ui_ready(const ink_ui_canvas_t *canvas) {
    return canvas != nullptr && canvas->priv.frame != nullptr;
}

uint8_t ui_brightness(ink_ui_color_t color) {
    const uint32_t b = ((uint32_t)color.red * 77 + (uint32_t)color.green * 150 + (uint32_t)color.blue * 29) >> 8;
    return (uint8_t)(b > 255 ? 255 : b);
}

void ui_apply_solid_paint(ink_ui_canvas_t *canvas, ink_ui_paint_t paint) {
    uint8_t color;
    if (paint.mode == INK_UI_PAINT_MODE_XOR) {
        color = 2;
    } else {
        color = ui_brightness(paint.color) < 128 ? 0 : 1;
    }
    u8g2_SetDrawColor(&ui_screen(canvas)->u8g2, color);
}

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
