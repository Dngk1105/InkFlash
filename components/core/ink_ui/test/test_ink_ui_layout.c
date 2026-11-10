/* Design: docs/design/ui.md */

#include "ink_ui.h"
#include "unity.h"

void setUp(void) {
}

void tearDown(void) {
}

/* Purpose: positive insets must shrink a rect and negative insets must grow it around the same center.
 * Does: insets a 100 × 80 px rect by (5, 10) and (-5, -10), verifying the resulting positions and dimensions. */
static void test_rect_inset_normal(void) {
    const ink_ui_rect_t rect   = {.x = 10, .y = 20, .width = 100, .height = 80};
    const ink_ui_rect_t shrunk = ink_ui_rect_inset(rect, 5, 10);
    TEST_ASSERT_EQUAL_INT16(15, shrunk.x);
    TEST_ASSERT_EQUAL_INT16(30, shrunk.y);
    TEST_ASSERT_EQUAL_INT16(90, shrunk.width);
    TEST_ASSERT_EQUAL_INT16(60, shrunk.height);

    const ink_ui_rect_t grown = ink_ui_rect_inset(rect, -5, -10);
    TEST_ASSERT_EQUAL_INT16(5, grown.x);
    TEST_ASSERT_EQUAL_INT16(10, grown.y);
    TEST_ASSERT_EQUAL_INT16(110, grown.width);
    TEST_ASSERT_EQUAL_INT16(100, grown.height);
}

/* Purpose: insets larger than half the rect size must clamp width and height to zero.
 * Does: insets a 20 × 30 px rect by (15, 20) and checks width and height are 0. */
static void test_rect_inset_oversized(void) {
    const ink_ui_rect_t rect   = {.x = 10, .y = 20, .width = 20, .height = 30};
    const ink_ui_rect_t shrunk = ink_ui_rect_inset(rect, 15, 20);
    TEST_ASSERT_EQUAL_INT16(0, shrunk.width);
    TEST_ASSERT_EQUAL_INT16(0, shrunk.height);
    TEST_ASSERT_TRUE(shrunk.width >= 0);
    TEST_ASSERT_TRUE(shrunk.height >= 0);
}

/* Purpose: splitting a row into fixed and flex slots must distribute remaining space proportionally after gaps and fixed lengths.
 * Does: splits a 100 × 20 px rect into fixed 20 px and flex 1:3 slots with a 4 px gap, checking resulting x and width of each slot. */
static void test_rect_split_row_example1(void) {
    const ink_ui_rect_t   rect      = {.x = 0, .y = 0, .width = 100, .height = 20};
    const ink_ui_length_t lengths[] = {{.fixed = 20}, {.flex = 1}, {.flex = 3}};
    ink_ui_rect_t         out[3];

    ink_ui_rect_split(rect, INK_UI_AXIS_ROW, 4, lengths, 3, out);

    TEST_ASSERT_EQUAL_INT16(0, out[0].x);
    TEST_ASSERT_EQUAL_INT16(20, out[0].width);
    TEST_ASSERT_EQUAL_INT16(24, out[1].x);
    TEST_ASSERT_EQUAL_INT16(18, out[1].width);
    TEST_ASSERT_EQUAL_INT16(46, out[2].x);
    TEST_ASSERT_EQUAL_INT16(54, out[2].width);

    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT16(0, out[i].y);
        TEST_ASSERT_EQUAL_INT16(20, out[i].height);
    }
}

/* Purpose: splitting along the column axis must divide height and vertical positions identically to row splitting.
 * Does: splits a 40 × 100 px rect into fixed 20 px and flex 1:3 slots with a 4 px gap along the column axis, verifying y positions and
 * heights. */
static void test_rect_split_column(void) {
    const ink_ui_rect_t   rect      = {.x = 10, .y = 5, .width = 40, .height = 100};
    const ink_ui_length_t lengths[] = {{.fixed = 20}, {.flex = 1}, {.flex = 3}};
    ink_ui_rect_t         out[3];

    ink_ui_rect_split(rect, INK_UI_AXIS_COLUMN, 4, lengths, 3, out);

    TEST_ASSERT_EQUAL_INT16(5, out[0].y);
    TEST_ASSERT_EQUAL_INT16(20, out[0].height);
    TEST_ASSERT_EQUAL_INT16(29, out[1].y);
    TEST_ASSERT_EQUAL_INT16(18, out[1].height);
    TEST_ASSERT_EQUAL_INT16(51, out[2].y);
    TEST_ASSERT_EQUAL_INT16(54, out[2].height);

    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT16(10, out[i].x);
        TEST_ASSERT_EQUAL_INT16(40, out[i].width);
    }
}

/* Purpose: integer division remainder in flex distribution must be absorbed by the last flex slot.
 * Does: splits a 100 px row equally across three flex 1 slots without gaps, checking widths are 33, 33, and 34 px. */
static void test_rect_split_rounding_remainder(void) {
    const ink_ui_rect_t   rect      = {.x = 0, .y = 0, .width = 100, .height = 10};
    const ink_ui_length_t lengths[] = {{.flex = 1}, {.flex = 1}, {.flex = 1}};
    ink_ui_rect_t         out[3];

    ink_ui_rect_split(rect, INK_UI_AXIS_ROW, 0, lengths, 3, out);

    TEST_ASSERT_EQUAL_INT16(33, out[0].width);
    TEST_ASSERT_EQUAL_INT16(33, out[1].width);
    TEST_ASSERT_EQUAL_INT16(34, out[2].width);
}

/* Purpose: a fixed slot that does not fit must not spill outside the rect.
 * Does: splits a 50 px row into three 30 px fixed slots and checks slot widths are cut to 30, 20, and 0 px. */
static void test_rect_split_fixed_overflow(void) {
    const ink_ui_rect_t   rect      = {.x = 0, .y = 0, .width = 50, .height = 10};
    const ink_ui_length_t lengths[] = {{.fixed = 30}, {.fixed = 30}, {.fixed = 30}};
    ink_ui_rect_t         out[3];

    ink_ui_rect_split(rect, INK_UI_AXIS_ROW, 0, lengths, 3, out);

    TEST_ASSERT_EQUAL_INT16(0, out[0].x);
    TEST_ASSERT_EQUAL_INT16(30, out[0].width);
    TEST_ASSERT_EQUAL_INT16(30, out[1].x);
    TEST_ASSERT_EQUAL_INT16(20, out[1].width);
    TEST_ASSERT_EQUAL_INT16(50, out[2].x);
    TEST_ASSERT_EQUAL_INT16(0, out[2].width);
}

/* Purpose: splitting must safely do nothing when count is zero, lengths is NULL, or out_rects is NULL.
 * Does: calls ink_ui_rect_split with zero count, NULL lengths, and NULL out_rects, verifying output rects are untouched. */
static void test_rect_split_null_or_zero(void) {
    ink_ui_rect_t out[2] = {
        {.x = 1, .y = 2, .width = 3, .height = 4},
        {.x = 5, .y = 6, .width = 7, .height = 8}
    };
    const ink_ui_length_t lengths[] = {{.fixed = 10}, {.fixed = 10}};

    ink_ui_rect_split((ink_ui_rect_t){0, 0, 50, 50}, INK_UI_AXIS_ROW, 0, lengths, 0, out);
    TEST_ASSERT_EQUAL_INT16(1, out[0].x);

    ink_ui_rect_split((ink_ui_rect_t){0, 0, 50, 50}, INK_UI_AXIS_ROW, 0, nullptr, 2, out);
    TEST_ASSERT_EQUAL_INT16(1, out[0].x);

    ink_ui_rect_split((ink_ui_rect_t){0, 0, 50, 50}, INK_UI_AXIS_ROW, 0, lengths, 2, nullptr);
}

/* Purpose: rect alignment must place a box correctly at START, CENTER, and END along both horizontal and vertical axes.
 * Does: aligns a 40 × 20 px box inside a 100 × 80 px rect at start, center, and end, checking resulting x, y, width, and height. */
static void test_rect_align_both_axes(void) {
    const ink_ui_rect_t outer = {.x = 10, .y = 20, .width = 100, .height = 80};
    const ink_ui_size_t size  = {.width = 40, .height = 20};

    const ink_ui_rect_t start = ink_ui_rect_align(outer, size, INK_UI_ALIGN_START, INK_UI_ALIGN_START);
    TEST_ASSERT_EQUAL_INT16(10, start.x);
    TEST_ASSERT_EQUAL_INT16(20, start.y);
    TEST_ASSERT_EQUAL_INT16(40, start.width);
    TEST_ASSERT_EQUAL_INT16(20, start.height);

    const ink_ui_rect_t center = ink_ui_rect_align(outer, size, INK_UI_ALIGN_CENTER, INK_UI_ALIGN_CENTER);
    TEST_ASSERT_EQUAL_INT16(40, center.x);
    TEST_ASSERT_EQUAL_INT16(50, center.y);
    TEST_ASSERT_EQUAL_INT16(40, center.width);
    TEST_ASSERT_EQUAL_INT16(20, center.height);

    const ink_ui_rect_t end = ink_ui_rect_align(outer, size, INK_UI_ALIGN_END, INK_UI_ALIGN_END);
    TEST_ASSERT_EQUAL_INT16(70, end.x);
    TEST_ASSERT_EQUAL_INT16(80, end.y);
    TEST_ASSERT_EQUAL_INT16(40, end.width);
    TEST_ASSERT_EQUAL_INT16(20, end.height);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rect_inset_normal);
    RUN_TEST(test_rect_inset_oversized);
    RUN_TEST(test_rect_split_row_example1);
    RUN_TEST(test_rect_split_column);
    RUN_TEST(test_rect_split_rounding_remainder);
    RUN_TEST(test_rect_split_fixed_overflow);
    RUN_TEST(test_rect_split_null_or_zero);
    RUN_TEST(test_rect_align_both_axes);
    return UNITY_END();
}
