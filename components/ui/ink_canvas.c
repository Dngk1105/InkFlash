#include <string.h>
#include "ink_canvas.h"

void ink_canvas_init(ink_canvas_t *c, uint8_t *buf, uint16_t w, uint16_t h)
{
    c->buf = buf;
    c->w = w;
    c->h = h;
    c->stride = (uint16_t)((w + 7) / 8);
}

void ink_canvas_clear(ink_canvas_t *c, ink_color_t color)
{
    memset(c->buf, color == INK_WHITE ? 0xFF : 0x00, (size_t)c->stride * c->h);
}

void ink_canvas_pixel(ink_canvas_t *c, int x, int y, ink_color_t color)
{
    if (x < 0 || y < 0 || x >= c->w || y >= c->h) {
        return;
    }
    uint8_t *p = &c->buf[y * c->stride + (x >> 3)];
    const uint8_t mask = (uint8_t)(0x80 >> (x & 7));
    *p = color == INK_WHITE ? (uint8_t)(*p | mask) : (uint8_t)(*p & ~mask);
}

bool ink_canvas_get(const ink_canvas_t *c, int x, int y)
{
    if (x < 0 || y < 0 || x >= c->w || y >= c->h) {
        return true;
    }
    return (c->buf[y * c->stride + (x >> 3)] >> (7 - (x & 7))) & 1;
}

void ink_canvas_hline(ink_canvas_t *c, int x, int y, int w, ink_color_t color)
{
    for (int i = 0; i < w; i++) {
        ink_canvas_pixel(c, x + i, y, color);
    }
}

void ink_canvas_vline(ink_canvas_t *c, int x, int y, int h, ink_color_t color)
{
    for (int i = 0; i < h; i++) {
        ink_canvas_pixel(c, x, y + i, color);
    }
}

void ink_canvas_fill_rect(ink_canvas_t *c, int x, int y, int w, int h, ink_color_t color)
{
    for (int j = 0; j < h; j++) {
        ink_canvas_hline(c, x, y + j, w, color);
    }
}

void ink_canvas_rect(ink_canvas_t *c, int x, int y, int w, int h, ink_color_t color)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    ink_canvas_hline(c, x, y, w, color);
    ink_canvas_hline(c, x, y + h - 1, w, color);
    ink_canvas_vline(c, x, y, h, color);
    ink_canvas_vline(c, x + w - 1, y, h, color);
}
