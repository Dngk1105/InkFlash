#include <string.h>
#include "ink_canvas.h"
#include "unity.h"

#define W 416
#define H 240

static uint8_t s_buf[(W + 7) / 8 * H];
static ink_canvas_t s_c;

void setUp(void)
{
    ink_canvas_init(&s_c, s_buf, W, H);
    ink_canvas_clear(&s_c, INK_WHITE);
}

void tearDown(void) {}

/* Định dạng phải khớp HAL màn: bit 1 = trắng, MSB là điểm bên trái. */
static void test_pixel_format_matches_hal(void)
{
    ink_canvas_pixel(&s_c, 0, 0, INK_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x7F, s_buf[0]);
    ink_canvas_pixel(&s_c, 7, 0, INK_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x7E, s_buf[0]);
    ink_canvas_pixel(&s_c, 8, 1, INK_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x7F, s_buf[s_c.stride + 1]);
}

static void test_clipping_does_not_write_outside(void)
{
    ink_canvas_fill_rect(&s_c, -10, -10, W + 20, 5, INK_BLACK); /* phủ ra ngoài mép trên */
    ink_canvas_pixel(&s_c, W, 0, INK_BLACK);
    ink_canvas_pixel(&s_c, 0, H, INK_BLACK);
    for (int x = 0; x < W; x++) {
        TEST_ASSERT_TRUE(ink_canvas_get(&s_c, x, 5)); /* hàng 5 vẫn trắng */
    }
}

static void test_rect_outline(void)
{
    ink_canvas_rect(&s_c, 10, 10, 5, 4, INK_BLACK);
    TEST_ASSERT_FALSE(ink_canvas_get(&s_c, 10, 10));
    TEST_ASSERT_FALSE(ink_canvas_get(&s_c, 14, 13));
    TEST_ASSERT_TRUE(ink_canvas_get(&s_c, 12, 11)); /* trong lòng còn trắng */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pixel_format_matches_hal);
    RUN_TEST(test_clipping_does_not_write_outside);
    RUN_TEST(test_rect_outline);
    return UNITY_END();
}
