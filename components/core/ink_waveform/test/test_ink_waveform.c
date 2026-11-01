/* Thiết kế: docs/design/display/display.md */

/* Host test: chạy trên PC bằng gcc, dùng Unity lấy từ ESP-IDF. Mỗi trường hợp một hàm test_*. */
#include "ink_waveform.h"
#include "unity.h"
#include <string.h>

void setUp(void) {
}

void tearDown(void) {
}

/* Bảng một nhóm, một pha. */
static void set_single_phase(uint8_t *table, ink_waveform_level_t level, uint8_t frames) {
    memset(table, 0, INK_WAVEFORM_TABLE_BYTES);
    table[0] = 1;
    table[1] = (uint8_t)((level << 6) | frames);
    table[5] = 1;
    table[6] = 1;
}

/* TR8 đọc từ OTP (waveform.md): LUTKK là 0 V 14 khung rồi VSH 1 khung. */
static ink_waveform_t tr8_like(void) {
    ink_waveform_t waveform;
    set_single_phase(waveform.tables[INK_WAVEFORM_TABLE_VCOM], INK_WAVEFORM_LEVEL_ZERO, 15);
    set_single_phase(waveform.tables[INK_WAVEFORM_TABLE_WHITE_TO_WHITE], INK_WAVEFORM_LEVEL_ZERO, 15);
    set_single_phase(waveform.tables[INK_WAVEFORM_TABLE_BLACK_TO_WHITE], INK_WAVEFORM_LEVEL_VSL, 15);
    set_single_phase(waveform.tables[INK_WAVEFORM_TABLE_WHITE_TO_BLACK], INK_WAVEFORM_LEVEL_VSH, 15);
    uint8_t *black_to_black = waveform.tables[INK_WAVEFORM_TABLE_BLACK_TO_BLACK];
    set_single_phase(black_to_black, INK_WAVEFORM_LEVEL_ZERO, 14);
    black_to_black[2] = (uint8_t)((INK_WAVEFORM_LEVEL_VSH << 6) | 1);
    return waveform;
}

static void test_build_rejects_bad_parameters(void) {
    ink_waveform_t                  waveform;
    const ink_waveform_parameters_t zero_frames = {.frames = 0, .hold_frames = 0};
    const ink_waveform_parameters_t too_long    = {.frames = 64, .hold_frames = 0};
    const ink_waveform_parameters_t long_hold   = {.frames = 4, .hold_frames = 5};
    TEST_ASSERT_EQUAL(INK_WAVEFORM_ERROR_ARGUMENT, ink_waveform_build(nullptr, &waveform));
    TEST_ASSERT_EQUAL(INK_WAVEFORM_ERROR_ARGUMENT, ink_waveform_build(&zero_frames, &waveform));
    TEST_ASSERT_EQUAL(INK_WAVEFORM_ERROR_ARGUMENT, ink_waveform_build(&too_long, &waveform));
    TEST_ASSERT_EQUAL(INK_WAVEFORM_ERROR_ARGUMENT, ink_waveform_build(&long_hold, &waveform));
}

static void test_build_six_frames_is_balanced(void) {
    ink_waveform_t                  waveform;
    const ink_waveform_parameters_t parameters = {.frames = 6, .hold_frames = 0};
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_build(&parameters, &waveform));
    TEST_ASSERT_EQUAL_UINT32(6, ink_waveform_frames(&waveform));
    ink_waveform_charge_t charge;
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_charge(&waveform, &charge));
    TEST_ASSERT_EQUAL_INT32(0, charge.cycle);
    TEST_ASSERT_EQUAL_INT32(0, charge.hold_white);
    TEST_ASSERT_EQUAL_INT32(0, charge.hold_black);
}

static void test_build_hold_pulse_keeps_length(void) {
    ink_waveform_t                  waveform;
    const ink_waveform_parameters_t parameters = {.frames = 10, .hold_frames = 2};
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_build(&parameters, &waveform));
    TEST_ASSERT_EQUAL_UINT32(10, ink_waveform_frames(&waveform));
    ink_waveform_charge_t charge;
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_charge(&waveform, &charge));
    TEST_ASSERT_EQUAL_INT32(0, charge.cycle);
    TEST_ASSERT_EQUAL_INT32(-2, charge.hold_white);
    TEST_ASSERT_EQUAL_INT32(2, charge.hold_black);
}

static void test_tr8_cycle_balanced_hold_black_warns(void) {
    const ink_waveform_t waveform = tr8_like();
    TEST_ASSERT_EQUAL_UINT32(15, ink_waveform_frames(&waveform));
    ink_waveform_charge_t charge;
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_charge(&waveform, &charge));
    TEST_ASSERT_EQUAL_INT32(0, charge.cycle);
    TEST_ASSERT_EQUAL_INT32(0, charge.hold_white);
    TEST_ASSERT_EQUAL_INT32(1, charge.hold_black);
}

static void test_unequal_push_is_unbalanced(void) {
    ink_waveform_t                  waveform;
    const ink_waveform_parameters_t parameters = {.frames = 6, .hold_frames = 0};
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_build(&parameters, &waveform));
    set_single_phase(waveform.tables[INK_WAVEFORM_TABLE_BLACK_TO_WHITE], INK_WAVEFORM_LEVEL_VSL, 4);
    ink_waveform_charge_t charge;
    TEST_ASSERT_EQUAL(INK_WAVEFORM_OK, ink_waveform_charge(&waveform, &charge));
    TEST_ASSERT_EQUAL_INT32(2, charge.cycle);
}

static void test_frames_count_repeats(void) {
    uint8_t table[INK_WAVEFORM_TABLE_BYTES];
    set_single_phase(table, INK_WAVEFORM_LEVEL_VSH, 5);
    table[0] = 3;                                            /* lặp nhóm */
    table[3] = (uint8_t)((INK_WAVEFORM_LEVEL_VSL << 6) | 2); /* trạng thái 2 */
    table[6] = 2;                                            /* lặp trạng thái 2 */
    TEST_ASSERT_EQUAL_UINT32((5 + 2 * 2) * 3, ink_waveform_table_frames(table));
}

static void test_zero_repeat_counts_once(void) {
    uint8_t table[INK_WAVEFORM_TABLE_BYTES];
    set_single_phase(table, INK_WAVEFORM_LEVEL_VSH, 7);
    table[0] = 0;
    table[5] = 0;
    TEST_ASSERT_EQUAL_UINT32(7, ink_waveform_table_frames(table));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_build_rejects_bad_parameters);
    RUN_TEST(test_build_six_frames_is_balanced);
    RUN_TEST(test_build_hold_pulse_keeps_length);
    RUN_TEST(test_tr8_cycle_balanced_hold_black_warns);
    RUN_TEST(test_unequal_push_is_unbalanced);
    RUN_TEST(test_frames_count_repeats);
    RUN_TEST(test_zero_repeat_counts_once);
    return UNITY_END();
}
