/* Thiết kế: docs/design/ui-canvas.md */
#include "ink_ui.h"

#define REPLACEMENT_CODEPOINT 0x3F /* '?' */

/* ---- Canvas ---- */

ink_ui_error_t ink_ui_canvas_init(ink_ui_canvas_t *canvas, uint8_t *buffer,
                                  size_t size, ink_ui_rotation_t rotation) {
  if (canvas == nullptr || buffer == nullptr || size < INK_UI_BUFFER_SIZE) {
    return INK_UI_ERROR_ARGUMENT;
  }
  if (rotation != INK_UI_ROTATION_90 && rotation != INK_UI_ROTATION_270) {
    return INK_UI_ERROR_ARGUMENT;
  }
  canvas->priv.buffer = buffer;
  canvas->priv.rotation = rotation;
  return INK_UI_OK;
}

void ink_ui_clear(ink_ui_canvas_t *canvas, ink_ui_color_t color) {
  if (canvas == nullptr) {
    return;
  }
  const uint8_t fill = color == INK_UI_COLOR_WHITE ? 0xFF : 0x00;
  for (size_t i = 0; i < INK_UI_BUFFER_SIZE; i++) {
    canvas->priv.buffer[i] = fill;
  }
}

/* Toạ độ đã nằm trong màn. */
static void put_pixel(ink_ui_canvas_t *canvas, int32_t x, int32_t y,
                      ink_ui_color_t color) {
  int32_t column;
  int32_t row;
  if (canvas->priv.rotation == INK_UI_ROTATION_90) {
    column = y;
    row = INK_UI_PANEL_HEIGHT - 1 - x;
  } else {
    column = INK_UI_PANEL_WIDTH - 1 - y;
    row = x;
  }
  uint8_t *byte =
      &canvas->priv.buffer[row * INK_UI_PANEL_ROW_BYTES + (column >> 3)];
  const uint8_t mask = (uint8_t)(0x80 >> (column & 7));
  if (color == INK_UI_COLOR_WHITE) {
    *byte |= mask;
  } else {
    *byte &= (uint8_t)~mask;
  }
}

void ink_ui_set_pixel(ink_ui_canvas_t *canvas, int32_t x, int32_t y,
                      ink_ui_color_t color) {
  if (canvas == nullptr || x < 0 || y < 0 || x >= INK_UI_WIDTH ||
      y >= INK_UI_HEIGHT) {
    return;
  }
  put_pixel(canvas, x, y, color);
}

void ink_ui_fill_rect(ink_ui_canvas_t *canvas, int32_t x, int32_t y,
                      int32_t width, int32_t height, ink_ui_color_t color) {
  if (canvas == nullptr || width <= 0 || height <= 0) {
    return;
  }
  const int64_t left = x < 0 ? 0 : x;
  const int64_t top = y < 0 ? 0 : y;
  const int64_t right =
      (int64_t)x + width > INK_UI_WIDTH ? INK_UI_WIDTH : (int64_t)x + width;
  const int64_t bottom =
      (int64_t)y + height > INK_UI_HEIGHT ? INK_UI_HEIGHT : (int64_t)y + height;
  for (int64_t row = top; row < bottom; row++) {
    for (int64_t column = left; column < right; column++) {
      put_pixel(canvas, (int32_t)column, (int32_t)row, color);
    }
  }
}

void ink_ui_draw_rect(ink_ui_canvas_t *canvas, int32_t x, int32_t y,
                      int32_t width, int32_t height, ink_ui_color_t color) {
  if (width <= 0 || height <= 0) {
    return;
  }
  ink_ui_fill_rect(canvas, x, y, width, 1, color);
  ink_ui_fill_rect(canvas, x, y + height - 1, width, 1, color);
  ink_ui_fill_rect(canvas, x, y + 1, 1, height - 2, color);
  ink_ui_fill_rect(canvas, x + width - 1, y + 1, 1, height - 2, color);
}

/* ---- Giải mã UTF-8 ---- */

static bool is_continuation(uint8_t byte) { return (byte & 0xC0) == 0x80; }

/*
 * Trả mã Unicode tại *text và đẩy *text qua đoạn đã đọc. Đoạn lỗi trả '?' và
 * nuốt cả các byte tiếp theo nó, để một ký tự hỏng chỉ ra một '?'.
 */
static uint32_t decode_next(const char **text) {
  const uint8_t *p = (const uint8_t *)*text;
  const uint8_t lead = p[0];
  int extra = 0;
  uint32_t codepoint;
  uint32_t minimum;
  if (lead < 0x80) {
    *text += 1;
    return lead;
  } else if (lead >= 0xC2 && lead <= 0xDF) {
    extra = 1;
    codepoint = lead & 0x1Fu;
    minimum = 0x80;
  } else if (lead >= 0xE0 && lead <= 0xEF) {
    extra = 2;
    codepoint = lead & 0x0Fu;
    minimum = 0x800;
  } else if (lead >= 0xF0 && lead <= 0xF4) {
    extra = 3;
    codepoint = lead & 0x07u;
    minimum = 0x10000;
  } else {
    extra = 0; /* byte tiếp lẻ hoặc byte đầu không dùng trong UTF-8 */
    codepoint = 0;
    minimum = 0;
  }

  int used = 1;
  while (used < 1 + (extra > 0 ? extra : 3) && is_continuation(p[used])) {
    codepoint = (codepoint << 6) | (p[used] & 0x3Fu);
    used++;
  }
  *text += used;
  if (extra == 0 || used != 1 + extra) {
    return REPLACEMENT_CODEPOINT;
  }
  if (codepoint < minimum || codepoint > 0x10FFFF ||
      (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
    return REPLACEMENT_CODEPOINT;
  }
  return codepoint;
}

/* ---- Chữ ---- */

static const ink_ui_glyph_t *find_glyph(const ink_ui_font_t *font,
                                        uint32_t codepoint) {
  if (codepoint > UINT16_MAX) {
    return nullptr;
  }
  uint32_t low = 0;
  uint32_t high = font->glyph_count;
  while (low < high) {
    const uint32_t middle = (low + high) / 2;
    if (font->glyphs[middle].codepoint < codepoint) {
      low = middle + 1;
    } else {
      high = middle;
    }
  }
  if (low < font->glyph_count && font->glyphs[low].codepoint == codepoint) {
    return &font->glyphs[low];
  }
  return nullptr;
}

static const ink_ui_glyph_t *glyph_or_fallback(const ink_ui_font_t *font,
                                               uint32_t codepoint) {
  const ink_ui_glyph_t *glyph = find_glyph(font, codepoint);
  return glyph != nullptr ? glyph : find_glyph(font, REPLACEMENT_CODEPOINT);
}

static void draw_glyph(ink_ui_canvas_t *canvas, const ink_ui_font_t *font,
                       const ink_ui_glyph_t *glyph, int32_t x, int32_t y,
                       ink_ui_color_t color) {
  const uint8_t *bitmap = &font->bitmaps[glyph->bitmap_offset];
  const int32_t row_bytes = (glyph->width + 7) / 8;
  const int32_t left = x + glyph->offset_x;
  const int32_t top = y + font->ascent + glyph->offset_y;
  if (left >= INK_UI_WIDTH || top >= INK_UI_HEIGHT ||
      left + glyph->width <= 0 || top + glyph->height <= 0) {
    return;
  }
  for (int32_t row = 0; row < glyph->height; row++) {
    for (int32_t column = 0; column < glyph->width; column++) {
      if (bitmap[row * row_bytes + (column >> 3)] & (0x80 >> (column & 7))) {
        ink_ui_set_pixel(canvas, left + column, top + row, color);
      }
    }
  }
}

int32_t ink_ui_draw_text(ink_ui_canvas_t *canvas, const ink_ui_font_t *font,
                         int32_t x, int32_t y, const char *text,
                         ink_ui_color_t color) {
  if (canvas == nullptr || font == nullptr || text == nullptr) {
    return 0;
  }
  int32_t cursor = 0;
  while (*text != '\0') {
    const ink_ui_glyph_t *glyph = glyph_or_fallback(font, decode_next(&text));
    if (glyph == nullptr) {
      continue;
    }
    draw_glyph(canvas, font, glyph, x + cursor, y, color);
    cursor += glyph->advance;
  }
  return cursor;
}

int32_t ink_ui_measure_text(const ink_ui_font_t *font, const char *text) {
  if (font == nullptr || text == nullptr) {
    return 0;
  }
  int32_t width = 0;
  while (*text != '\0') {
    const ink_ui_glyph_t *glyph = glyph_or_fallback(font, decode_next(&text));
    if (glyph != nullptr) {
      width += glyph->advance;
    }
  }
  return width;
}
