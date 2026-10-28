/* Thiết kế: docs/design/display-pipeline.md */

/* Host test: chạy trên PC bằng gcc, dùng Unity lấy từ ESP-IDF. Mỗi trường hợp một hàm test_*. */
#include <string.h>
#include "ink_display_policy.h"
#include "unity.h"

/* Màn nhỏ 20 × 4: mỗi hàng 3 byte, 4 bit đệm cuối hàng. */
#define WIDTH     20
#define HEIGHT    4
#define ROW_BYTES 3
#define SIZE      (ROW_BYTES * HEIGHT)

static uint8_t s_shown[SIZE];
static uint8_t s_next[SIZE];

static ink_display_policy_config_t default_config(void) {
    return (ink_display_policy_config_t){
        .width                = WIDTH,
        .height               = HEIGHT,
        .has_partial          = true,
        .x_align              = 8,
        .partials_before_full = 3,
        .max_partial_percent  = 100,
    };
}

/* Khởi tạo và vẽ lần đầu (FULL), để màn có khung hiện là s_shown. */
static void start(ink_display_policy_state_t *state, const ink_display_policy_config_t *config) {
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(state, config));
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_decide(state, nullptr, s_shown, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_FULL, decision.mode);
}

static void set_black(uint8_t *frame, uint32_t x, uint32_t y) {
    frame[y * ROW_BYTES + x / 8] &= (uint8_t)~(0x80U >> (x % 8));
}

static ink_display_policy_decision_t decide_next(ink_display_policy_state_t *state) {
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_decide(state, s_shown, s_next, &decision));
    return decision;
}

void setUp(void) {
    memset(s_shown, 0xFF, SIZE);
    memset(s_next, 0xFF, SIZE);
}

void tearDown(void) {
}

static void test_init_rejects_bad_config(void) {
    ink_display_policy_state_t  state;
    ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(nullptr, &config));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&state, nullptr));
    config.x_align = 0;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&state, &config));
    config                      = default_config();
    config.partials_before_full = 0;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&state, &config));
    config                     = default_config();
    config.max_partial_percent = 101;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&state, &config));
}

static void test_first_decide_is_full_screen(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&state, &config));
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_decide(&state, nullptr, s_next, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_FULL, decision.mode);
    TEST_ASSERT_EQUAL(0, decision.window.x);
    TEST_ASSERT_EQUAL(0, decision.window.y);
    TEST_ASSERT_EQUAL(WIDTH, decision.window.width);
    TEST_ASSERT_EQUAL(HEIGHT, decision.window.height);
}

static void test_decide_rejects_missing_shown(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_decide(&state, nullptr, s_next, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_decide(&state, s_shown, nullptr, &decision));
}

static void test_same_frame_is_none(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_NONE, decide_next(&state).mode);
}

static void test_padding_bits_are_ignored(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    s_next[ROW_BYTES - 1] = 0xF0; /* chỉ đổi 4 bit đệm sau điểm x = 19 */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_NONE, decide_next(&state).mode);
}

static void test_partial_window_is_aligned_outward(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    set_black(s_next, 9, 1);
    set_black(s_next, 10, 2);
    const ink_display_policy_decision_t decision = decide_next(&state);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_PARTIAL, decision.mode);
    TEST_ASSERT_EQUAL(8, decision.window.x);
    TEST_ASSERT_EQUAL(1, decision.window.y);
    TEST_ASSERT_EQUAL(8, decision.window.width);
    TEST_ASSERT_EQUAL(2, decision.window.height);
}

static void test_aligned_window_stops_at_screen_edge(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    set_black(s_next, 19, 3);
    const ink_display_policy_decision_t decision = decide_next(&state);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_PARTIAL, decision.mode);
    TEST_ASSERT_EQUAL(16, decision.window.x);
    TEST_ASSERT_EQUAL(4, decision.window.width);
}

static void test_full_after_partial_threshold(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    for (uint32_t i = 0; i < 3; i++) {
        s_next[0] ^= 0x80;
        TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_PARTIAL, decide_next(&state).mode);
        memcpy(s_shown, s_next, SIZE);
    }
    s_next[0] ^= 0x80;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_FULL, decide_next(&state).mode);
    memcpy(s_shown, s_next, SIZE);
    s_next[0] ^= 0x80;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_PARTIAL, decide_next(&state).mode);
}

static void test_full_when_window_too_large(void) {
    ink_display_policy_state_t  state;
    ink_display_policy_config_t config = default_config();
    config.max_partial_percent         = 50;
    start(&state, &config);
    set_black(s_next, 0, 0);
    set_black(s_next, 8, 3); /* vùng 16 × 4 = 80 % màn */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_FULL, decide_next(&state).mode);
}

static void test_full_without_partial(void) {
    ink_display_policy_state_t  state;
    ink_display_policy_config_t config = default_config();
    config.has_partial                 = false;
    start(&state, &config);
    set_black(s_next, 0, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_FULL, decide_next(&state).mode);
}

static void test_invalidate_forces_full(void) {
    ink_display_policy_state_t        state;
    const ink_display_policy_config_t config = default_config();
    start(&state, &config);
    ink_display_policy_invalidate(&state);
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_decide(&state, nullptr, s_next, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_MODE_FULL, decision.mode);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_config);
    RUN_TEST(test_first_decide_is_full_screen);
    RUN_TEST(test_decide_rejects_missing_shown);
    RUN_TEST(test_same_frame_is_none);
    RUN_TEST(test_padding_bits_are_ignored);
    RUN_TEST(test_partial_window_is_aligned_outward);
    RUN_TEST(test_aligned_window_stops_at_screen_edge);
    RUN_TEST(test_full_after_partial_threshold);
    RUN_TEST(test_full_when_window_too_large);
    RUN_TEST(test_full_without_partial);
    RUN_TEST(test_invalidate_forces_full);
    return UNITY_END();
}
