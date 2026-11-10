/* Design: docs/design/ui.md */

#include "ink_ui.h"
#include "unity.h"

#include <string.h>

#define PANEL_WIDTH  240
#define PANEL_HEIGHT 416
#define ROW_BYTES    (PANEL_WIDTH / 8)
#define FRAME_BYTES  (ROW_BYTES * PANEL_HEIGHT)

static const ink_display_frame_info_t s_info = {
    .width  = PANEL_WIDTH,
    .height = PANEL_HEIGHT,
    .format = INK_DISPLAY_PIXEL_FORMAT_K1,
};

static ink_ui_canvas_t s_canvas;
static uint8_t         s_frame[FRAME_BYTES];

void setUp(void) {
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&s_canvas, &s_info, INK_UI_ROTATION_0));
    memset(s_frame, 0xFF, sizeof(s_frame));
    ink_ui_canvas_bind(&s_canvas, s_frame);
}

void tearDown(void) {
}

/* Purpose: font line height must return a positive pixel value for both GNU Unifont and Fixed fonts.
 * Does: calls ink_ui_text_line_height for unifont_vi_16 (16 px) and fixed_10, asserting both return heights greater than zero. */
static void test_line_height_positive(void) {
    const int16_t h_unifont = ink_ui_text_line_height(&s_canvas, &ink_ui_font_unifont_vi_16);
    TEST_ASSERT_TRUE(h_unifont > 0);
    TEST_ASSERT_EQUAL_INT16(16, h_unifont);

    const int16_t h_fixed = ink_ui_text_line_height(&s_canvas, &ink_ui_font_fixed_10);
    TEST_ASSERT_TRUE(h_fixed > 0);
}

/* Purpose: GNU Unifont font data must include glyphs for all Vietnamese accented vowels and consonants.
 * Does: measures each Vietnamese character individually with ink_ui_text_measure and asserts its width is greater than zero. */
static void test_vietnamese_glyphs_present(void) {
    const char               *viet_chars = "ăâđêôơư áàảãạ ấầẩẫậ ắằẳẵặ ếềểễệ ốồổỗộ ớờởỡợ ứừửữự ýỳỷỹỵ ĂÂĐÊÔƠƯ";
    const ink_ui_text_style_t style      = {
             .font      = &ink_ui_font_unifont_vi_16,
             .align     = INK_UI_ALIGN_START,
             .max_lines = 1,
             .line_gap  = 0,
    };

    const char *p = viet_chars;
    while (*p != '\0') {
        if (*p == ' ') {
            p++;
            continue;
        }
        size_t        char_len = 1;
        const uint8_t c        = (uint8_t)*p;
        if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }

        char one_char[8];
        memcpy(one_char, p, char_len);
        one_char[char_len] = '\0';

        const ink_ui_size_t sz = ink_ui_text_measure(&s_canvas, one_char, 100, &style);
        TEST_ASSERT_TRUE_MESSAGE(sz.width > 0, one_char);

        p += char_len;
    }
}

/* Purpose: measuring text with large width must produce a single line, while narrow width must wrap text into multiple lines.
 * Does: measures a phrase at 1000 px and half width, asserting the narrow measure has greater height and width within limit. */
static void test_example5_measure_wrap(void) {
    const char               *text  = "Xin chào thế giới";
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 0,
        .line_gap  = 0,
    };

    /* Measure with huge max_width -> single line, height = line_height */
    const ink_ui_size_t single_sz = ink_ui_text_measure(&s_canvas, text, 1000, &style);
    TEST_ASSERT_EQUAL_INT16(ink_ui_text_line_height(&s_canvas, &ink_ui_font_unifont_vi_16), single_sz.height);
    TEST_ASSERT_TRUE(single_sz.width > 0);

    /* Measure with max_width smaller than full text width -> multiple lines, none wider than max_width */
    const int16_t       narrow_w = (int16_t)(single_sz.width / 2);
    const ink_ui_size_t multi_sz = ink_ui_text_measure(&s_canvas, text, narrow_w, &style);
    TEST_ASSERT_TRUE(multi_sz.height > single_sz.height);
    TEST_ASSERT_TRUE(multi_sz.width <= narrow_w);
}

/* Purpose: newline characters in text must force line breaks regardless of available width.
 * Does: measures a 3-line string separated by newlines with 500 px width and asserts total height equals three line heights. */
static void test_newline_forces_break(void) {
    const char               *text  = "Dòng 1\nDòng 2\nDòng 3";
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 0,
        .line_gap  = 0,
    };
    const int16_t       lh = ink_ui_text_line_height(&s_canvas, &ink_ui_font_unifont_vi_16);
    const ink_ui_size_t sz = ink_ui_text_measure(&s_canvas, text, 500, &style);
    TEST_ASSERT_EQUAL_INT16(lh * 3, sz.height);
}

/* Purpose: words wider than available width must break between characters without splitting multi-byte UTF-8 sequences.
 * Does: measures a 14-letter Vietnamese word with 40 px width, checking height exceeds one line and width stays within 40 px. */
static void test_long_word_breaks_between_utf8_chars(void) {
    /* Long word in Vietnamese without spaces, wider than 40 px */
    const char               *long_word = "Nghiêngnghiêng";
    const ink_ui_text_style_t style     = {
            .font      = &ink_ui_font_unifont_vi_16,
            .align     = INK_UI_ALIGN_START,
            .max_lines = 0,
            .line_gap  = 0,
    };
    const int16_t       lh = ink_ui_text_line_height(&s_canvas, &ink_ui_font_unifont_vi_16);
    const ink_ui_size_t sz = ink_ui_text_measure(&s_canvas, long_word, 40, &style);
    TEST_ASSERT_TRUE(sz.height > lh);
    TEST_ASSERT_TRUE(sz.width <= 40);
}

/* Purpose: style max_lines must limit the measured height to at most that number of lines.
 * Does: measures a 5-line string with max_lines set to 2 and verifies height equals exactly two line heights. */
static void test_max_lines_limits_measure(void) {
    const char   *text = "Một\nHai\nBa\nBốn\nNăm";
    const int16_t lh   = ink_ui_text_line_height(&s_canvas, &ink_ui_font_unifont_vi_16);

    const ink_ui_text_style_t style2 = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 2,
        .line_gap  = 0,
    };
    const ink_ui_size_t sz2 = ink_ui_text_measure(&s_canvas, text, 200, &style2);
    TEST_ASSERT_EQUAL_INT16(lh * 2, sz2.height);
}

/* Purpose: drawing text into a rect shorter than total text height must draw only lines that fit within the rect.
 * Does: draws 4 lines into a rect sized for 2 lines and verifies the returned drawn size height equals two line heights. */
static void test_draw_capped_by_rect_height(void) {
    const char               *text  = "Dòng 1\nDòng 2\nDòng 3\nDòng 4";
    const int16_t             lh    = ink_ui_text_line_height(&s_canvas, &ink_ui_font_unifont_vi_16);
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 0,
        .line_gap  = 0,
    };
    /* Rect only tall enough for 2 lines: lh * 2 */
    const ink_ui_rect_t rect     = {.x = 0, .y = 0, .width = 200, .height = (int16_t)(lh * 2 + 2)};
    const ink_ui_size_t drawn_sz = ink_ui_text_draw(&s_canvas, rect, text, &style, INK_UI_PAINT_BLACK);
    TEST_ASSERT_EQUAL_INT16(lh * 2, drawn_sz.height);
}

/* Purpose: drawing text that fits entirely within a rect must return the exact same size as ink_ui_text_measure.
 * Does: measures and draws a short phrase inside a 200 × 100 px rect, checking width and height match between measure and draw. */
static void test_draw_matches_measure_when_fits(void) {
    const char               *text  = "Vừa vặn trong khung";
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 0,
        .line_gap  = 0,
    };
    const ink_ui_size_t measured = ink_ui_text_measure(&s_canvas, text, 200, &style);
    const ink_ui_rect_t rect     = {.x = 10, .y = 10, .width = 200, .height = 100};
    const ink_ui_size_t drawn    = ink_ui_text_draw(&s_canvas, rect, text, &style, INK_UI_PAINT_BLACK);
    TEST_ASSERT_EQUAL_INT16(measured.width, drawn.width);
    TEST_ASSERT_EQUAL_INT16(measured.height, drawn.height);
}

/* Purpose: ink_ui_text_draw must not modify any pixels outside the specified bounding rectangle.
 * Does: fills a frame with 0xFF, draws text inside a rect, verifies all outer rows and columns stay 0xFF, and verifies inner pixels
 * changed. */
static void test_draw_pixels_only_inside_rect(void) {
    memset(s_frame, 0xFF, sizeof(s_frame));

    const ink_ui_rect_t       rect  = {.x = 24, .y = 30, .width = 80, .height = 30};
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_START,
        .max_lines = 1,
        .line_gap  = 0,
    };

    ink_ui_text_draw(&s_canvas, rect, "Thử nghiệm", &style, INK_UI_PAINT_BLACK);

    /* Verify pixels outside rect are unchanged (0xFF) */
    /* Rows before rect.y */
    for (int y = 0; y < rect.y; y++) {
        for (int b = 0; b < ROW_BYTES; b++) { TEST_ASSERT_EQUAL_HEX8(0xFF, s_frame[y * ROW_BYTES + b]); }
    }
    /* Rows after rect.y + rect.height */
    for (int y = rect.y + rect.height; y < PANEL_HEIGHT; y++) {
        for (int b = 0; b < ROW_BYTES; b++) { TEST_ASSERT_EQUAL_HEX8(0xFF, s_frame[y * ROW_BYTES + b]); }
    }
    /* Bytes before rect.x (bytes 0, 1, 2 = pixels 0..23) on the lines of the rect */
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int b = 0; b < rect.x / 8; b++) { TEST_ASSERT_EQUAL_HEX8(0xFF, s_frame[y * ROW_BYTES + b]); }
    }

    /* Verify at least one pixel changed inside rect */
    bool changed = false;
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int b = rect.x / 8; b < (rect.x + rect.width) / 8; b++) {
            if (s_frame[y * ROW_BYTES + b] != 0xFF) {
                changed = true;
                break;
            }
        }
        if (changed) { break; }
    }
    TEST_ASSERT_TRUE(changed);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_line_height_positive);
    RUN_TEST(test_vietnamese_glyphs_present);
    RUN_TEST(test_example5_measure_wrap);
    RUN_TEST(test_newline_forces_break);
    RUN_TEST(test_long_word_breaks_between_utf8_chars);
    RUN_TEST(test_max_lines_limits_measure);
    RUN_TEST(test_draw_capped_by_rect_height);
    RUN_TEST(test_draw_matches_measure_when_fits);
    RUN_TEST(test_draw_pixels_only_inside_rect);
    return UNITY_END();
}
