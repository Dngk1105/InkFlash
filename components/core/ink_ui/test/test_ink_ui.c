/* Thiết kế: docs/design/ui-canvas.md */

#include <string.h>
#include "ink_ui.h"
#include "ink_ui_font_cascadia_code_24.h"
#include "unity.h"

static uint8_t         s_buffer[INK_UI_BUFFER_SIZE];
static ink_ui_canvas_t s_canvas;

void setUp(void) {
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&s_canvas, s_buffer, sizeof s_buffer, INK_UI_ROTATION_90));
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
}

void tearDown(void) {
}

/* Bit của điểm panel (cột, hàng): 1 là trắng. */
static int panel_bit(int column, int row) {
    return (s_buffer[row * INK_UI_PANEL_ROW_BYTES + column / 8] >> (7 - column % 8)) & 1;
}

static int count_black(void) {
    int count = 0;
    for (int row = 0; row < INK_UI_PANEL_HEIGHT; row++) {
        for (int column = 0; column < INK_UI_PANEL_WIDTH; column++) { count += !panel_bit(column, row); }
    }
    return count;
}

/* ---- Khởi tạo, xoá ---- */

static void test_init_rejects_bad_arguments(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(nullptr, s_buffer, sizeof s_buffer, INK_UI_ROTATION_90));
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(&canvas, nullptr, sizeof s_buffer, INK_UI_ROTATION_90));
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(&canvas, s_buffer, INK_UI_BUFFER_SIZE - 1, INK_UI_ROTATION_90));
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(&canvas, s_buffer, sizeof s_buffer, (ink_ui_rotation_t)7));
}

static void test_clear_fills_every_byte(void) {
    ink_ui_clear(&s_canvas, INK_UI_COLOR_BLACK);
    for (size_t i = 0; i < sizeof s_buffer; i++) { TEST_ASSERT_EQUAL_HEX8(0x00, s_buffer[i]); }
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    for (size_t i = 0; i < sizeof s_buffer; i++) { TEST_ASSERT_EQUAL_HEX8(0xFF, s_buffer[i]); }
}

/* ---- Đặt điểm và ánh xạ khi xoay ---- */

static void test_set_pixel_rotation_90_maps_corners(void) {
    ink_ui_set_pixel(&s_canvas, 0, 0, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(0, 415));
    ink_ui_set_pixel(&s_canvas, 415, 0, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(0, 0));
    ink_ui_set_pixel(&s_canvas, 0, 239, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(239, 415));
    ink_ui_set_pixel(&s_canvas, 415, 239, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(239, 0));
    TEST_ASSERT_EQUAL(4, count_black());
}

static void test_set_pixel_rotation_270_maps_corners(void) {
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&s_canvas, s_buffer, sizeof s_buffer, INK_UI_ROTATION_270));
    ink_ui_set_pixel(&s_canvas, 0, 0, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(239, 0));
    ink_ui_set_pixel(&s_canvas, 415, 0, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(239, 415));
    ink_ui_set_pixel(&s_canvas, 0, 239, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(0, 0));
    ink_ui_set_pixel(&s_canvas, 415, 239, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, panel_bit(0, 415));
    TEST_ASSERT_EQUAL(4, count_black());
}

static void test_set_pixel_bit_order_first_pixel_is_high_bit(void) {
    /* Xoay 90°: điểm (415, 0) là cột 0, hàng 0 => bit cao nhất của byte đầu. */
    ink_ui_set_pixel(&s_canvas, 415, 0, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x7F, s_buffer[0]);
    ink_ui_set_pixel(&s_canvas, 415, 7, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x7E, s_buffer[0]);
    ink_ui_set_pixel(&s_canvas, 415, 8, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x7F, s_buffer[1]);
}

static void test_set_pixel_white_restores_bit(void) {
    ink_ui_set_pixel(&s_canvas, 10, 10, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(1, count_black());
    ink_ui_set_pixel(&s_canvas, 10, 10, INK_UI_COLOR_WHITE);
    TEST_ASSERT_EQUAL(0, count_black());
}

/* ---- Cắt mép ---- */

static void test_set_pixel_outside_is_ignored(void) {
    ink_ui_set_pixel(&s_canvas, -1, 0, INK_UI_COLOR_BLACK);
    ink_ui_set_pixel(&s_canvas, 0, -1, INK_UI_COLOR_BLACK);
    ink_ui_set_pixel(&s_canvas, INK_UI_WIDTH, 0, INK_UI_COLOR_BLACK);
    ink_ui_set_pixel(&s_canvas, 0, INK_UI_HEIGHT, INK_UI_COLOR_BLACK);
    ink_ui_set_pixel(&s_canvas, INT32_MIN, INT32_MAX, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, count_black());
}

static void test_fill_rect_clips_at_edges(void) {
    ink_ui_fill_rect(&s_canvas, -5, -5, 10, 10, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(25, count_black());
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_fill_rect(&s_canvas, 410, 235, 100, 100, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(30, count_black());
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_fill_rect(&s_canvas, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX, INK_UI_COLOR_BLACK);
    ink_ui_fill_rect(&s_canvas, 0, 0, 0, 5, INK_UI_COLOR_BLACK);
}

static void test_fill_rect_whole_screen(void) {
    ink_ui_fill_rect(&s_canvas, 0, 0, INK_UI_WIDTH, INK_UI_HEIGHT, INK_UI_COLOR_BLACK);
    for (size_t i = 0; i < sizeof s_buffer; i++) { TEST_ASSERT_EQUAL_HEX8(0x00, s_buffer[i]); }
}

static void test_draw_rect_is_one_pixel_border(void) {
    ink_ui_draw_rect(&s_canvas, 10, 20, 8, 5, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(2 * 8 + 2 * 3, count_black());
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_draw_rect(&s_canvas, 10, 20, 1, 1, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(1, count_black());
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_draw_rect(&s_canvas, 10, 20, 5, 1, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(5, count_black());
}

static void test_drawing_with_null_canvas_does_nothing(void) {
    ink_ui_clear(nullptr, INK_UI_COLOR_BLACK);
    ink_ui_set_pixel(nullptr, 0, 0, INK_UI_COLOR_BLACK);
    ink_ui_fill_rect(nullptr, 0, 0, 5, 5, INK_UI_COLOR_BLACK);
    ink_ui_draw_rect(nullptr, 0, 0, 5, 5, INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, ink_ui_draw_text(nullptr, &ink_ui_font_cascadia_code_24, 0, 0, "a", INK_UI_COLOR_BLACK));
}

/* ---- Chữ ---- */

static const uint16_t ADVANCE = 14; /* Cascadia Code là font đơn cách */

static int draw_and_count(const char *text) {
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_draw_text(&s_canvas, &ink_ui_font_cascadia_code_24, 20, 20, text, INK_UI_COLOR_BLACK);
    return count_black();
}

static void test_font_table_is_sorted_and_has_fallback(void) {
    const ink_ui_font_t *font              = &ink_ui_font_cascadia_code_24;
    bool                 has_question_mark = false;
    for (int i = 1; i < font->glyph_count; i++) { TEST_ASSERT_TRUE(font->glyphs[i - 1].codepoint < font->glyphs[i].codepoint); }
    for (int i = 0; i < font->glyph_count; i++) { has_question_mark |= font->glyphs[i].codepoint == '?'; }
    TEST_ASSERT_TRUE(has_question_mark);
}

static void test_draw_text_returns_width_and_matches_measure(void) {
    const char *text = "Hello";
    TEST_ASSERT_EQUAL(5 * ADVANCE, ink_ui_measure_text(&ink_ui_font_cascadia_code_24, text));
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    TEST_ASSERT_EQUAL(5 * ADVANCE, ink_ui_draw_text(&s_canvas, &ink_ui_font_cascadia_code_24, 20, 20, text, INK_UI_COLOR_BLACK));
    TEST_ASSERT_TRUE(count_black() > 0);
}

static void test_draw_text_empty_and_space_draw_nothing(void) {
    TEST_ASSERT_EQUAL(0, draw_and_count(""));
    TEST_ASSERT_EQUAL(0, draw_and_count("   "));
    TEST_ASSERT_EQUAL(0, ink_ui_measure_text(&ink_ui_font_cascadia_code_24, ""));
}

static void test_draw_text_clips_at_edges(void) {
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    int32_t width = ink_ui_draw_text(&s_canvas, &ink_ui_font_cascadia_code_24, 400, 230, "WWWW", INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(4 * ADVANCE, width);
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_draw_text(&s_canvas, &ink_ui_font_cascadia_code_24, -1000, -1000, "WWWW", INK_UI_COLOR_BLACK);
    ink_ui_draw_text(&s_canvas, &ink_ui_font_cascadia_code_24, 1000, 1000, "WWWW", INK_UI_COLOR_BLACK);
    TEST_ASSERT_EQUAL(0, count_black());
}

static void test_draw_text_accented_letter_has_more_ink_than_base(void) {
    /* Ậ = A + dấu mũ + dấu nặng: nhiều điểm đen hơn A, và dấu mũ nằm trên đỉnh chữ A. */
    const int base   = draw_and_count("A");
    const int accent = draw_and_count("\xE1\xBA\xAC"); /* U+1EAC */
    TEST_ASSERT_TRUE(base > 0);
    TEST_ASSERT_TRUE(accent > base);
    TEST_ASSERT_EQUAL(ADVANCE, ink_ui_measure_text(&ink_ui_font_cascadia_code_24, "\xE1\xBA\xAC"));

    /* Chữ cao nhất không bị cắt khi y = 0 vì dòng đã tính phần dấu. */
    ink_ui_clear(&s_canvas, INK_UI_COLOR_WHITE);
    ink_ui_draw_text(&s_canvas, &ink_ui_font_cascadia_code_24, 0, 0, "\xE1\xBA\xAC", INK_UI_COLOR_BLACK);
    const int at_top = count_black();
    TEST_ASSERT_EQUAL(accent, at_top);
}

static void test_draw_text_vietnamese_sentence_all_distinct_from_fallback(void) {
    const char *sentence = "Tiếng Việt: ưu tiên, đặc biệt, ngữ nghĩa";
    TEST_ASSERT_TRUE(draw_and_count(sentence) > 0);
    /* Không ký tự nào trong câu bị thay bằng '?': độ rộng bằng số ký tự (không phải số byte) nhân bước tiến. */
    TEST_ASSERT_EQUAL(40 * ADVANCE, ink_ui_measure_text(&ink_ui_font_cascadia_code_24, sentence));
}

/* ---- UTF-8 ---- */

static void expect_as_question_mark(const char *text, int units) {
    TEST_ASSERT_EQUAL(units * ADVANCE, ink_ui_measure_text(&ink_ui_font_cascadia_code_24, text));
    TEST_ASSERT_EQUAL(draw_and_count("?"), draw_and_count(text) / units);
}

static void test_utf8_missing_character_draws_question_mark(void) {
    expect_as_question_mark("\xE2\x82\xAC", 1);     /* € không có trong font */
    expect_as_question_mark("\xF0\x9F\x98\x80", 1); /* ngoài BMP */
    expect_as_question_mark("\xCC\x81", 1);         /* dấu sắc rời U+0301 */
}

static void test_utf8_invalid_bytes_draw_one_question_mark_each_segment(void) {
    expect_as_question_mark("\x80", 1);     /* byte tiếp đứng lẻ */
    expect_as_question_mark("\xC0\xAF", 1); /* mã dài thừa */
    expect_as_question_mark("\xFF", 1);
    expect_as_question_mark("\xE1\xBA", 1);         /* thiếu byte cuối, hết chuỗi */
    expect_as_question_mark("\xED\xA0\x80", 1);     /* surrogate */
    expect_as_question_mark("\xF4\x90\x80\x80", 1); /* lớn hơn U+10FFFF */
}

static void test_utf8_invalid_byte_does_not_break_following_text(void) {
    const int expected = draw_and_count("a?b");
    TEST_ASSERT_EQUAL(expected,
                      draw_and_count("a\xFF"
                                     "b"));
    TEST_ASSERT_EQUAL(expected,
                      draw_and_count("a\xE1\xBA"
                                     "b")); /* thiếu byte, byte kế là ASCII */
    TEST_ASSERT_EQUAL(3 * ADVANCE,
                      ink_ui_measure_text(&ink_ui_font_cascadia_code_24,
                                          "a\xE1\xBA"
                                          "b"));
}

static void test_utf8_valid_multibyte_in_font_is_not_question_mark(void) {
    TEST_ASSERT_TRUE(draw_and_count("\xC4\x91") != draw_and_count("?"));     /* đ */
    TEST_ASSERT_TRUE(draw_and_count("\xE1\xBB\x85") != draw_and_count("?")); /* ễ */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_arguments);
    RUN_TEST(test_clear_fills_every_byte);
    RUN_TEST(test_set_pixel_rotation_90_maps_corners);
    RUN_TEST(test_set_pixel_rotation_270_maps_corners);
    RUN_TEST(test_set_pixel_bit_order_first_pixel_is_high_bit);
    RUN_TEST(test_set_pixel_white_restores_bit);
    RUN_TEST(test_set_pixel_outside_is_ignored);
    RUN_TEST(test_fill_rect_clips_at_edges);
    RUN_TEST(test_fill_rect_whole_screen);
    RUN_TEST(test_draw_rect_is_one_pixel_border);
    RUN_TEST(test_drawing_with_null_canvas_does_nothing);
    RUN_TEST(test_font_table_is_sorted_and_has_fallback);
    RUN_TEST(test_draw_text_returns_width_and_matches_measure);
    RUN_TEST(test_draw_text_empty_and_space_draw_nothing);
    RUN_TEST(test_draw_text_clips_at_edges);
    RUN_TEST(test_draw_text_accented_letter_has_more_ink_than_base);
    RUN_TEST(test_draw_text_vietnamese_sentence_all_distinct_from_fallback);
    RUN_TEST(test_utf8_missing_character_draws_question_mark);
    RUN_TEST(test_utf8_invalid_bytes_draw_one_question_mark_each_segment);
    RUN_TEST(test_utf8_invalid_byte_does_not_break_following_text);
    RUN_TEST(test_utf8_valid_multibyte_in_font_is_not_question_mark);
    return UNITY_END();
}
