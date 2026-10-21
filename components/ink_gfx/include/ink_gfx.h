#pragma once

#include <stdarg.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * Vẽ lên framebuffer 1 bit, đúng định dạng của HAL màn hình (ink_hal_display.h):
 * 1 bit/điểm ảnh, bit 1 = trắng, bit 0 = đen, MSB là điểm bên trái, mỗi hàng ceil(w/8) byte.
 * Không biết gì về phần cứng: dùng được với mọi màn, mọi kích thước.
 */
typedef struct {
    uint8_t *buf;
    uint16_t w, h;
    uint16_t stride; /* byte mỗi hàng */
} ink_fb_t;

typedef enum { INK_BLACK = 0, INK_WHITE = 1 } ink_color_t;

#define INK_FONT_W 5
#define INK_FONT_H 7
/* Một ký tự chiếm (INK_FONT_W + 1) × scale điểm theo chiều ngang, INK_FONT_H × scale theo chiều dọc. */
#define INK_TEXT_ADVANCE(scale) ((INK_FONT_W + 1) * (scale))
#define INK_TEXT_HEIGHT(scale)  (INK_FONT_H * (scale))

extern const uint8_t ink_font5x7[95][5]; /* ASCII 32–126 */

esp_err_t ink_fb_init(ink_fb_t *fb, uint16_t w, uint16_t h); /* cấp phát, xoá trắng */
void ink_fb_free(ink_fb_t *fb);
void ink_fb_clear(ink_fb_t *fb, ink_color_t color);
void ink_fb_pixel(ink_fb_t *fb, int x, int y, ink_color_t color);
void ink_fb_fill_rect(ink_fb_t *fb, int x, int y, int w, int h, ink_color_t color);
void ink_fb_draw_rect(ink_fb_t *fb, int x, int y, int w, int h, ink_color_t color); /* viền 1 điểm */
void ink_fb_hline(ink_fb_t *fb, int x, int y, int w, ink_color_t color);
/* (x, y) là góc trên trái của ký tự đầu tiên. Ký tự ngoài ASCII 32–126 vẽ thành '?'. */
void ink_fb_text(ink_fb_t *fb, int x, int y, int scale, ink_color_t color, const char *s);
void ink_fb_printf(ink_fb_t *fb, int x, int y, int scale, ink_color_t color, const char *fmt, ...)
    __attribute__((format(printf, 6, 7)));
