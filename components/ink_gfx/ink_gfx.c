#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ink_gfx.h"

esp_err_t ink_fb_init(ink_fb_t *fb, uint16_t w, uint16_t h)
{
    fb->w = w;
    fb->h = h;
    fb->stride = (w + 7) / 8;
    fb->buf = malloc((size_t)fb->stride * h);
    if (!fb->buf) {
        return ESP_ERR_NO_MEM;
    }
    ink_fb_clear(fb, INK_WHITE);
    return ESP_OK;
}

void ink_fb_free(ink_fb_t *fb)
{
    free(fb->buf);
    fb->buf = NULL;
}

void ink_fb_clear(ink_fb_t *fb, ink_color_t color)
{
    memset(fb->buf, color == INK_WHITE ? 0xFF : 0x00, (size_t)fb->stride * fb->h);
}

void ink_fb_pixel(ink_fb_t *fb, int x, int y, ink_color_t color)
{
    if (x < 0 || y < 0 || x >= fb->w || y >= fb->h) {
        return;
    }
    uint8_t *p = &fb->buf[y * fb->stride + (x >> 3)];
    const uint8_t mask = 0x80 >> (x & 7);
    if (color == INK_WHITE) {
        *p |= mask;
    } else {
        *p &= ~mask;
    }
}

void ink_fb_hline(ink_fb_t *fb, int x, int y, int w, ink_color_t color)
{
    for (int i = 0; i < w; i++) {
        ink_fb_pixel(fb, x + i, y, color);
    }
}

void ink_fb_fill_rect(ink_fb_t *fb, int x, int y, int w, int h, ink_color_t color)
{
    for (int j = 0; j < h; j++) {
        ink_fb_hline(fb, x, y + j, w, color);
    }
}

void ink_fb_draw_rect(ink_fb_t *fb, int x, int y, int w, int h, ink_color_t color)
{
    ink_fb_hline(fb, x, y, w, color);
    ink_fb_hline(fb, x, y + h - 1, w, color);
    for (int j = 0; j < h; j++) {
        ink_fb_pixel(fb, x, y + j, color);
        ink_fb_pixel(fb, x + w - 1, y + j, color);
    }
}

void ink_fb_text(ink_fb_t *fb, int x, int y, int scale, ink_color_t color, const char *s)
{
    if (scale < 1) {
        scale = 1;
    }
    for (; *s; s++, x += INK_TEXT_ADVANCE(scale)) {
        unsigned char c = (unsigned char)*s;
        const uint8_t *glyph = ink_font5x7[(c >= 32 && c <= 126 ? c : '?') - 32];
        for (int col = 0; col < INK_FONT_W; col++) {
            for (int row = 0; row < INK_FONT_H; row++) {
                if (glyph[col] & (1 << row)) {
                    ink_fb_fill_rect(fb, x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

void ink_fb_printf(ink_fb_t *fb, int x, int y, int scale, ink_color_t color, const char *fmt, ...)
{
    char line[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    ink_fb_text(fb, x, y, scale, color, line);
}
