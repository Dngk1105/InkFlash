/* Thiết kế: docs/design/display/display.md */

/* Host test: chạy trên PC bằng gcc, dùng Unity lấy từ ESP-IDF. Mỗi trường hợp một hàm test_*. */
#include "ink_frame_slots.h"
#include "unity.h"

static ink_frame_slots_state_t s_state;

static ink_frame_slots_lease_t acquire(void) {
    ink_frame_slots_lease_t lease;
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_acquire(&s_state, &lease));
    return lease;
}

/* Mượn rồi giao; trả khung đã giao. */
static int8_t acquire_and_submit(void) {
    const int8_t frame = acquire().frame;
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_submit(&s_state));
    return frame;
}

/* Task màn lấy khung chờ ra rồi hiện nó. */
static int8_t take_and_show(void) {
    const int8_t frame = ink_frame_slots_take(&s_state);
    TEST_ASSERT_NOT_EQUAL(INK_FRAME_SLOTS_NONE, frame);
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_show(&s_state, frame));
    return frame;
}

void setUp(void) {
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_init(&s_state));
}

void tearDown(void) {
}

static void test_rejects_null(void) {
    ink_frame_slots_lease_t lease;
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_ARGUMENT, ink_frame_slots_init(nullptr));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_ARGUMENT, ink_frame_slots_acquire(nullptr, &lease));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_ARGUMENT, ink_frame_slots_acquire(&s_state, nullptr));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_ARGUMENT, ink_frame_slots_submit(nullptr));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_NONE, ink_frame_slots_take(nullptr));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_ARGUMENT, ink_frame_slots_show(&s_state, INK_FRAME_SLOTS_COUNT));
}

static void test_first_acquire_is_white(void) {
    const ink_frame_slots_lease_t lease = acquire();
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_FILL_WHITE, lease.fill);
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_NONE, lease.copy_from);
    TEST_ASSERT_EQUAL(lease.frame, ink_frame_slots_borrowed(&s_state));
}

static void test_acquire_twice_or_submit_unborrowed_is_state_error(void) {
    ink_frame_slots_lease_t lease;
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_STATE, ink_frame_slots_submit(&s_state));
    acquire();
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_STATE, ink_frame_slots_acquire(&s_state, &lease));
}

static void test_acquire_copies_latest_submitted(void) {
    const int8_t                  first = acquire_and_submit();
    const ink_frame_slots_lease_t lease = acquire();
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_FILL_COPY, lease.fill);
    TEST_ASSERT_EQUAL(first, lease.copy_from);
    TEST_ASSERT_NOT_EQUAL(first, lease.frame);
}

static void test_take_then_show_frees_previous_shown(void) {
    const int8_t first = acquire_and_submit();
    TEST_ASSERT_EQUAL(first, take_and_show());
    TEST_ASSERT_EQUAL(first, ink_frame_slots_shown(&s_state));

    const int8_t second = acquire_and_submit();
    TEST_ASSERT_EQUAL(second, take_and_show());
    TEST_ASSERT_EQUAL(second, ink_frame_slots_shown(&s_state));
    /* first đã trống: hai lần mượn tiếp theo không phải lấy lại khung chờ */
    acquire_and_submit();
    TEST_ASSERT_EQUAL(0, ink_frame_slots_dropped(&s_state));
}

static void test_submit_over_pending_drops_old(void) {
    const int8_t first  = acquire_and_submit();
    const int8_t second = acquire_and_submit();
    TEST_ASSERT_EQUAL(1, ink_frame_slots_dropped(&s_state));
    TEST_ASSERT_EQUAL(second, ink_frame_slots_take(&s_state));
    TEST_ASSERT_NOT_EQUAL(first, second);
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_NONE, ink_frame_slots_take(&s_state));
}

/* Task màn giữ khung đang hiện và khung đang vẽ: task main lấy lại khung chờ. */
static void test_acquire_reclaims_pending_when_display_holds_two(void) {
    acquire_and_submit();
    take_and_show();
    acquire_and_submit();
    const int8_t drawing = ink_frame_slots_take(&s_state);
    const int8_t pending = acquire_and_submit();

    const ink_frame_slots_lease_t lease = acquire();
    TEST_ASSERT_EQUAL(pending, lease.frame);
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_FILL_KEEP, lease.fill);
    TEST_ASSERT_EQUAL(1, ink_frame_slots_dropped(&s_state));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_NONE, ink_frame_slots_take(&s_state));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_show(&s_state, drawing));
}

static void test_show_rejects_frame_not_taken(void) {
    const int8_t pending = acquire_and_submit();
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_STATE, ink_frame_slots_show(&s_state, pending));
    const int8_t shown = take_and_show();
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_STATE, ink_frame_slots_show(&s_state, shown));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_STATE, ink_frame_slots_release(&s_state, shown));
}

static void test_release_keeps_image_for_next_acquire(void) {
    const int8_t frame = acquire_and_submit();
    TEST_ASSERT_EQUAL(frame, ink_frame_slots_take(&s_state));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_release(&s_state, frame));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_NONE, ink_frame_slots_shown(&s_state));

    const ink_frame_slots_lease_t lease = acquire();
    TEST_ASSERT_EQUAL(frame, lease.frame);
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_FILL_KEEP, lease.fill);
}

static void test_assume_shown_is_latest(void) {
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_OK, ink_frame_slots_assume_shown(&s_state, 0));
    TEST_ASSERT_EQUAL(0, ink_frame_slots_shown(&s_state));
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_ERROR_STATE, ink_frame_slots_assume_shown(&s_state, 0));

    const ink_frame_slots_lease_t lease = acquire();
    TEST_ASSERT_EQUAL(INK_FRAME_SLOTS_FILL_COPY, lease.fill);
    TEST_ASSERT_EQUAL(0, lease.copy_from);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rejects_null);
    RUN_TEST(test_first_acquire_is_white);
    RUN_TEST(test_acquire_twice_or_submit_unborrowed_is_state_error);
    RUN_TEST(test_acquire_copies_latest_submitted);
    RUN_TEST(test_take_then_show_frees_previous_shown);
    RUN_TEST(test_submit_over_pending_drops_old);
    RUN_TEST(test_acquire_reclaims_pending_when_display_holds_two);
    RUN_TEST(test_show_rejects_frame_not_taken);
    RUN_TEST(test_release_keeps_image_for_next_acquire);
    RUN_TEST(test_assume_shown_is_latest);
    return UNITY_END();
}
