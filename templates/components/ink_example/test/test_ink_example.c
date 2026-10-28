/* Thiết kế: docs/design/code-structure.md */

/* Host test: chạy trên PC bằng gcc, dùng Unity lấy từ ESP-IDF. Mỗi trường hợp một hàm test_*. */
#include "ink_example.h"
#include "unity.h"

void setUp(void) {
}

void tearDown(void) {
}

static void test_handle_rejects_null(void) {
    ink_example_state_t state;
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_init(&state, 1));
    ink_example_action_t action;
    TEST_ASSERT_EQUAL(INK_EXAMPLE_ERROR_ARGUMENT, ink_example_handle(&state, nullptr, &action));
}

static void test_init_rejects_zero_limit(void) {
    ink_example_state_t state;
    TEST_ASSERT_EQUAL(INK_EXAMPLE_ERROR_ARGUMENT, ink_example_init(&state, 0));
}

static void test_notify_after_limit_ticks(void) {
    ink_example_state_t state;
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_init(&state, 3));
    const ink_example_event_t tick = {.type = INK_EXAMPLE_EVENT_TICK, .time_ms = 0};
    ink_example_action_t      action;
    for (int i = 0; i < 2; i++) {
        TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_handle(&state, &tick, &action));
        TEST_ASSERT_EQUAL(INK_EXAMPLE_ACTION_NONE, action);
    }
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_handle(&state, &tick, &action));
    TEST_ASSERT_EQUAL(INK_EXAMPLE_ACTION_NOTIFY, action);
}

static void test_reset_clears_count(void) {
    ink_example_state_t state;
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_init(&state, 2));
    const ink_example_event_t tick  = {.type = INK_EXAMPLE_EVENT_TICK, .time_ms = 0};
    const ink_example_event_t reset = {.type = INK_EXAMPLE_EVENT_RESET, .time_ms = 0};
    ink_example_action_t      action;
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_handle(&state, &tick, &action));
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_handle(&state, &reset, &action));
    TEST_ASSERT_EQUAL(INK_EXAMPLE_OK, ink_example_handle(&state, &tick, &action));
    TEST_ASSERT_EQUAL(INK_EXAMPLE_ACTION_NONE, action);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_zero_limit);
    RUN_TEST(test_handle_rejects_null);
    RUN_TEST(test_notify_after_limit_ticks);
    RUN_TEST(test_reset_clears_count);
    return UNITY_END();
}
