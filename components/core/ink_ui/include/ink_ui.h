/* Thiết kế: docs/design/ui-canvas.md */
#pragma once

/*
 * Canvas 1 bit và vẽ chữ. C thuần (C23), không cấp phát: bên gọi đưa vùng nhớ khung vào ink_ui_canvas_init.
 */
#include <stddef.h>
#include <stdint.h>

/* ---- Kích thước ---- */

/* Bố cục ảnh của panel (dọc). */
#define INK_UI_PANEL_WIDTH     240
#define INK_UI_PANEL_HEIGHT    416
#define INK_UI_PANEL_ROW_BYTES 30
#define INK_UI_BUFFER_SIZE     12480

/* Toạ độ app (ngang). */
#define INK_UI_WIDTH  416
#define INK_UI_HEIGHT 240

/* ---- Kiểu ---- */

typedef enum : uint8_t {
    INK_UI_OK = 0,
    INK_UI_ERROR_ARGUMENT,
} ink_ui_error_t;

/* Giá trị số bằng đúng bit lưu trong ảnh. */
typedef enum : uint8_t {
    INK_UI_COLOR_BLACK = 0,
    INK_UI_COLOR_WHITE = 1,
} ink_ui_color_t;

typedef enum : uint8_t {
    INK_UI_ROTATION_90,
    INK_UI_ROTATION_270,
} ink_ui_rotation_t;

/* Bitmap mỗi hàng làm tròn lên byte, bit 1 là nét chữ. `offset_y` tính từ đường cơ sở, âm là phía trên. */
typedef struct {
    uint16_t codepoint;
    uint16_t bitmap_offset;
    uint8_t  width;
    uint8_t  height;
    int8_t   offset_x;
    int8_t   offset_y;
    uint8_t  advance;
} ink_ui_glyph_t;

/* Do tools/font_gen.py sinh. `glyphs` xếp tăng theo `codepoint`, phải có '?'. */
typedef struct {
    const ink_ui_glyph_t *glyphs;
    const uint8_t        *bitmaps;
    uint16_t              glyph_count;
    uint8_t               line_height;
    uint8_t               ascent; /* từ đỉnh dòng tới đường cơ sở */
} ink_ui_font_t;

typedef struct {
    struct {
        uint8_t          *buffer;
        ink_ui_rotation_t rotation;
    } priv;
} ink_ui_canvas_t;

/* ---- Canvas ---- */

/*
 * Gắn canvas vào vùng nhớ của bên gọi; không xoá nội dung.
 * Trả INK_UI_ERROR_ARGUMENT nếu có con trỏ nullptr, `size` < INK_UI_BUFFER_SIZE hoặc `rotation` không hợp lệ.
 */
[[nodiscard]] ink_ui_error_t ink_ui_canvas_init(ink_ui_canvas_t *canvas, uint8_t *buffer, size_t size,
                                                ink_ui_rotation_t rotation);

/* Các hàm vẽ cắt ở mép màn và bỏ qua canvas nullptr. Toạ độ theo màn ngang 416 × 240. */
void ink_ui_clear(ink_ui_canvas_t *canvas, ink_ui_color_t color);
void ink_ui_set_pixel(ink_ui_canvas_t *canvas, int32_t x, int32_t y, ink_ui_color_t color);
void ink_ui_fill_rect(ink_ui_canvas_t *canvas, int32_t x, int32_t y, int32_t width, int32_t height,
                      ink_ui_color_t color);

/* Viền dày 1 điểm nằm trong hình chữ nhật. */
void ink_ui_draw_rect(ink_ui_canvas_t *canvas, int32_t x, int32_t y, int32_t width, int32_t height,
                      ink_ui_color_t color);

/* ---- Chữ ---- */

/*
 * Vẽ chuỗi UTF-8 với (x, y) là góc trên trái của dòng; trả độ rộng đã vẽ (điểm), cả phần bị cắt.
 * Ký tự thiếu trong font và byte UTF-8 lỗi vẽ thành '?'. Không tự xuống dòng.
 */
int32_t ink_ui_draw_text(ink_ui_canvas_t *canvas, const ink_ui_font_t *font, int32_t x, int32_t y, const char *text,
                         ink_ui_color_t color);

/* Độ rộng ink_ui_draw_text sẽ vẽ, không vẽ gì. */
int32_t ink_ui_measure_text(const ink_ui_font_t *font, const char *text);
