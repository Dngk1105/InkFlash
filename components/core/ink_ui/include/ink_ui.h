/* Design: docs/design/ui.md */
#pragma once

/*
 * Drawing tools for apps: a canvas over one frame, primitives, paint, text and rulers. The app acquires the frame,
 * binds the canvas to it, draws, and submits; ui never acquires or submits and keeps no list of what was drawn.
 * Pure C (C23), no ESP-IDF include. Coordinates are px in the rotated orientation of the canvas, origin top left.
 */
#include "ink_display_types.h"
#include <stddef.h>
#include <stdint.h>

/* ---- Errors ---- */

typedef enum : uint8_t {
    INK_UI_OK = 0,
    INK_UI_ERROR_ARGUMENT, /**< pointer is NULL, size is 0, or value out of range */
    INK_UI_ERROR_FORMAT,   /**< pixel format of the frame is not supported, or the frame is too large */
} ink_ui_error_t;

/* ---- Geometry ---- */

/** Rectangle; empty when width or height is <= 0. */
typedef struct {
    int16_t x;      /**< top-left x in canvas coordinates, px */
    int16_t y;      /**< top-left y in canvas coordinates, px */
    int16_t width;  /**< width, px */
    int16_t height; /**< height, px */
} ink_ui_rect_t;

typedef struct {
    int16_t width;  /**< width, px */
    int16_t height; /**< height, px */
} ink_ui_size_t;

/** Clockwise rotation of the canvas against the native panel orientation. */
typedef enum : uint8_t {
    INK_UI_ROTATION_0 = 0, /**< 0° (native panel orientation) */
    INK_UI_ROTATION_90,    /**< 90° clockwise */
    INK_UI_ROTATION_180,   /**< 180° clockwise */
    INK_UI_ROTATION_270,   /**< 270° clockwise */
} ink_ui_rotation_t;

typedef enum : uint8_t {
    INK_UI_ALIGN_START = 0, /**< left or top */
    INK_UI_ALIGN_CENTER,    /**< centered along the axis */
    INK_UI_ALIGN_END,       /**< right or bottom */
} ink_ui_align_t;

/* ---- Paint ---- */

/** Wanted color; the canvas maps it to the pixel format of the frame. */
typedef struct {
    uint8_t red;   /**< red channel, 0..255 */
    uint8_t green; /**< green channel, 0..255 */
    uint8_t blue;  /**< blue channel, 0..255 */
} ink_ui_color_t;

typedef enum : uint8_t {
    INK_UI_PAINT_MODE_COPY = 0, /**< write the color */
    INK_UI_PAINT_MODE_XOR,      /**< invert the pixels; the color is ignored */
} ink_ui_paint_mode_t;

typedef struct {
    ink_ui_color_t      color; /**< color used in COPY mode */
    ink_ui_paint_mode_t mode;  /**< COPY or XOR */
} ink_ui_paint_t;

#define INK_UI_PAINT_BLACK \
    ((ink_ui_paint_t){     \
        .color = {0, 0, 0}, \
          .mode = INK_UI_PAINT_MODE_COPY \
    })
#define INK_UI_PAINT_WHITE \
    ((ink_ui_paint_t){     \
        .color = {255, 255, 255}, \
          .mode = INK_UI_PAINT_MODE_COPY \
    })
#define INK_UI_PAINT_XOR ((ink_ui_paint_t){.mode = INK_UI_PAINT_MODE_XOR})
/** Gray of brightness `level`: 0 = black, 255 = white. */
#define INK_UI_PAINT_GRAY(level) \
    ((ink_ui_paint_t){           \
        .color = {(level), (level), (level)}, \
          .mode = INK_UI_PAINT_MODE_COPY \
    })

/* ---- Canvas ---- */

#define INK_UI_CANVAS_SCREEN_BYTES 512 /**< room for the drawing backend; checked by static_assert in ink_ui_canvas.c */

/** One canvas. The caller owns the struct but must not touch `priv`.
 * WARN: do not copy or move after ink_ui_canvas_init(); the backend keeps pointers into the struct. */
typedef struct {
    struct {
        ink_display_frame_info_t info;
        ink_ui_rotation_t        rotation;
        uint8_t                 *frame; /* bound frame; nullptr until ink_ui_canvas_bind() */
        alignas(max_align_t) unsigned char screen[INK_UI_CANVAS_SCREEN_BYTES];
    } priv;
} ink_ui_canvas_t;

/**
 * @brief Set up a canvas for frames described by `info`; no frame bound yet, clip = whole canvas.
 * @par Called from
 * Main task, once after ink_display_frame_info().
 * @retval INK_UI_ERROR_ARGUMENT a pointer is NULL, a size is 0, or rotation is out of range.
 * @retval INK_UI_ERROR_FORMAT info->format is not INK_DISPLAY_PIXEL_FORMAT_K1, or the frame is too large for the backend.
 */
[[nodiscard]] ink_ui_error_t ink_ui_canvas_init(ink_ui_canvas_t *canvas, const ink_display_frame_info_t *info, ink_ui_rotation_t rotation);

/**
 * @brief Bind the canvas to `frame`; later draw calls write into it. The pixels of the frame are kept.
 * @par Called from
 * Main task, after every ink_display_acquire().
 * @param frame Frame laid out as the frame info given to ink_ui_canvas_init(); NULL unbinds.
 */
void ink_ui_canvas_bind(ink_ui_canvas_t *canvas, uint8_t *frame);

/**
 * @brief Whole drawable area after rotation.
 * @par Called from
 * Main task.
 * @return {0, 0, width, height}; empty if canvas is NULL.
 */
ink_ui_rect_t ink_ui_canvas_bounds(const ink_ui_canvas_t *canvas);

/**
 * @brief Limit drawing to `clip`; draw calls write no pixel outside it.
 * @par Called from
 * Main task.
 * @param clip Clipped to the canvas bounds; an empty rect resets the clip to the whole canvas.
 */
void ink_ui_canvas_set_clip(ink_ui_canvas_t *canvas, ink_ui_rect_t clip);

/**
 * @brief Bound frame, for code that writes pixels itself.
 * @par Called from
 * Main task.
 * @param out_info Frame info of the canvas (native panel orientation); may be NULL.
 * @return Frame, or NULL if none is bound or canvas is NULL.
 */
uint8_t *ink_ui_canvas_frame(const ink_ui_canvas_t *canvas, ink_display_frame_info_t *out_info);

/* ---- Primitives ---- */
/* Draw calls do nothing when canvas is NULL, no frame is bound, or the shape is empty. */

/**
 * @brief Fill a rectangle.
 *
 * A color between black and white becomes a 4 × 4 dither pattern anchored at the canvas origin.
 * @par Called from
 * Main task.
 * @param radius Corner radius, px; 0 = square corners. Ignored for dithered colors.
 */
void ink_ui_fill(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t radius, ink_ui_paint_t paint);

/**
 * @brief Draw the outline of a rectangle; the outline grows inward from the edge of `rect`.
 * @par Called from
 * Main task.
 * @param thickness Outline width, px; 0 draws nothing.
 * @param radius Corner radius of the outer edge, px; 0 = square corners.
 * @param paint A color between black and white draws black below 50 % brightness, white otherwise.
 */
void ink_ui_stroke(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, uint8_t thickness, uint8_t radius, ink_ui_paint_t paint);

/**
 * @brief Draw a straight line between two points, both included.
 * @par Called from
 * Main task.
 * @param thickness Line width, px; 0 draws nothing.
 * @param paint Mapped as in ink_ui_stroke().
 */
void ink_ui_line(ink_ui_canvas_t *canvas, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness, ink_ui_paint_t paint);

/** Pixel layout of a bitmap.
 * WARN: append only. */
typedef enum : uint8_t {
    INK_UI_BITMAP_FORMAT_INVALID = 0, /**< 0 is invalid */
    INK_UI_BITMAP_FORMAT_MASK1,       /**< 1 bit per pixel, row by row, each row padded to a byte, leftmost pixel in the most
                                           significant bit; 1 = drawn with the paint, 0 = left untouched */
    /* EXTEND(ui): gray and color images add their formats here. */
} ink_ui_bitmap_format_t;

typedef struct {
    uint16_t               width;  /**< width, px */
    uint16_t               height; /**< height, px */
    ink_ui_bitmap_format_t format; /**< pixel layout of the data */
    const uint8_t         *data;   /**< bitmap bytes, row by row */
} ink_ui_bitmap_t;

/**
 * @brief Draw a bitmap with its top left corner at (x, y).
 * @par Called from
 * Main task.
 * @param paint Mapped as in ink_ui_stroke().
 */
void ink_ui_bitmap(ink_ui_canvas_t *canvas, int16_t x, int16_t y, const ink_ui_bitmap_t *bitmap, ink_ui_paint_t paint);

/* ---- Text ---- */

/** One font; create with tools/u8g2_font.py. */
typedef struct {
    const uint8_t *data; /**< font data in U8g2 format */
} ink_ui_font_t;

extern const ink_ui_font_t ink_ui_font_unifont_vi_16; /**< GNU Unifont, 16 px line, Vietnamese */
extern const ink_ui_font_t ink_ui_font_fixed_10;      /**< X11 6x10, 10 px line, Latin-1 only */

typedef struct {
    const ink_ui_font_t *font;      /**< font used to measure and draw */
    ink_ui_align_t       align;     /**< horizontal position of each line in the rect */
    uint8_t              max_lines; /**< 0 = as many as needed (measure) or as many as fit the rect (draw) */
    int8_t               line_gap;  /**< extra px between lines; may be negative */
} ink_ui_text_style_t;

/**
 * @brief Line height of a font, px.
 * @par Called from
 * Main task.
 * @return Height of one line without line_gap; 0 if an argument is NULL.
 */
int16_t ink_ui_text_line_height(ink_ui_canvas_t *canvas, const ink_ui_font_t *font);

/**
 * @brief Size the text takes when laid out in `max_width`, without drawing.
 *
 * Same line breaking as ink_ui_text_draw(): UTF-8, break at spaces, '\n' forces a break, a word wider than
 * max_width breaks between characters.
 * @par Called from
 * Main task.
 * @param max_width Available width, px.
 * @return Width of the widest line and height of all lines (after max_lines); {0, 0} on a NULL argument or empty text.
 */
ink_ui_size_t ink_ui_text_measure(ink_ui_canvas_t *canvas, const char *text, int16_t max_width, const ink_ui_text_style_t *style);

/**
 * @brief Draw text inside `rect`, starting at its top edge.
 *
 * Lines that do not fit the height of `rect` or exceed max_lines are not drawn; when text is cut, the last drawn
 * line ends with "…". The background is not cleared.
 * @par Called from
 * Main task.
 * @param paint Mapped as in ink_ui_stroke().
 * @return Size actually drawn: widest line and total height; {0, 0} when nothing is drawn.
 */
ink_ui_size_t ink_ui_text_draw(ink_ui_canvas_t *canvas, ink_ui_rect_t rect, const char *text, const ink_ui_text_style_t *style,
                               ink_ui_paint_t paint);

/* ---- Rulers ---- */
/* Pure rectangle math; no canvas needed. */

typedef enum : uint8_t {
    INK_UI_AXIS_ROW = 0, /**< slots side by side, left to right */
    INK_UI_AXIS_COLUMN,  /**< slots stacked, top to bottom */
} ink_ui_axis_t;

/** Length of one slot along the split axis: `fixed` px plus a share of the free space proportional to `flex`. */
typedef struct {
    int16_t fixed; /**< fixed part, px */
    uint8_t flex;  /**< 0 = no share */
} ink_ui_length_t;

/**
 * @brief Shrink a rect by `horizontal` px on the left and right and `vertical` px on the top and bottom.
 * @par Called from
 * Any task.
 * @return Rect with the same center; width and height never below 0. Negative insets grow the rect.
 */
ink_ui_rect_t ink_ui_rect_inset(ink_ui_rect_t rect, int16_t horizontal, int16_t vertical);

/**
 * @brief Split a rect into `count` slots along `axis`, separated by `gap` px.
 *
 * Free space = length of the rect - gaps - sum of fixed parts; it is shared by flex weight and the last flex slot
 * takes the rounding remainder. When the fixed parts do not fit, later slots are cut and may become empty.
 * @par Called from
 * Any task.
 * @param out_rects `count` rects; each spans the whole rect across the axis. Untouched if a pointer is NULL.
 */
void ink_ui_rect_split(ink_ui_rect_t rect, ink_ui_axis_t axis, int16_t gap, const ink_ui_length_t *lengths, size_t count,
                       ink_ui_rect_t *out_rects);

/**
 * @brief Place a box of `size` inside `outer`.
 * @par Called from
 * Any task.
 * @return Rect of `size` positioned by the two alignments; it may stick out when `size` is larger than `outer`.
 */
ink_ui_rect_t ink_ui_rect_align(ink_ui_rect_t outer, ink_ui_size_t size, ink_ui_align_t horizontal, ink_ui_align_t vertical);
