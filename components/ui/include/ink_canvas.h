#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Canvas 1 bit/điểm ảnh, đúng định dạng framebuffer của HAL màn:
 * bit 1 = trắng, bit 0 = đen, MSB là điểm bên trái, mỗi hàng `stride` byte.
 * Canvas không tự cấp phát: người tạo đưa buffer vào (chọn RAM nội hay PSRAM là việc của người tạo).
 *
 * GĐ1 sẽ thêm: font bitmap UTF-8 có dấu tiếng Việt, xuống dòng, căn lề.
 */
typedef struct {
    uint8_t *buf;
    uint16_t w, h;
    uint16_t stride;
} ink_canvas_t;

typedef enum { INK_BLACK = 0, INK_WHITE = 1 } ink_color_t;

/* Số byte buffer cần cho canvas w×h. */
static inline uint32_t ink_canvas_bytes(uint16_t w, uint16_t h)
{
    return (uint32_t)((w + 7) / 8) * h;
}

void ink_canvas_init(ink_canvas_t *c, uint8_t *buf, uint16_t w, uint16_t h);
void ink_canvas_clear(ink_canvas_t *c, ink_color_t color);
void ink_canvas_pixel(ink_canvas_t *c, int x, int y, ink_color_t color);
bool ink_canvas_get(const ink_canvas_t *c, int x, int y); /* true = trắng */
void ink_canvas_hline(ink_canvas_t *c, int x, int y, int w, ink_color_t color);
void ink_canvas_vline(ink_canvas_t *c, int x, int y, int h, ink_color_t color);
void ink_canvas_fill_rect(ink_canvas_t *c, int x, int y, int w, int h, ink_color_t color);
void ink_canvas_rect(ink_canvas_t *c, int x, int y, int w, int h, ink_color_t color);
