/* Thiết kế: docs/design/input.md */

/* Host test (Unity). advance() gọi tick đúng từng deadline, như system với timer thật. */
#include "ink_gesture.h"
#include "unity.h"

#define LOG_MAX 32

static const ink_gesture_config_t CFG = {
    .debounce_ms = 20,
    .double_ms   = 300,
    .hold_ms     = 600,
    .combo_ms    = 100,
    .ab_short_ms = 2000,
    .ab_long_ms  = 8000,
};

static ink_gesture_state_t s_st;
static ink_gesture_t       s_log[LOG_MAX];
static int                 s_count;

static void collect(const ink_gesture_out_t *out)
{
    for (int i = 0; i < out->count; i++) {
        TEST_ASSERT_LESS_THAN(LOG_MAX, s_count);
        s_log[s_count++] = out->items[i];
    }
}

static void advance(uint32_t t)
{
    uint32_t deadline;
    for (int guard = 0; ink_gesture_next_deadline(&s_st, &deadline) && (int32_t)(t - deadline) >= 0; guard++) {
        TEST_ASSERT_LESS_THAN_MESSAGE(100, guard, "tick không tiêu hạn");
        ink_gesture_out_t out;
        TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_tick(&s_st, deadline, &out));
        collect(&out);
    }
}

static void edge(ink_gesture_btn_t btn, ink_gesture_level_t level, uint32_t t)
{
    advance(t);
    const ink_gesture_event_t ev = {.btn = btn, .level = level, .time_ms = t};
    ink_gesture_out_t         out;
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_feed(&s_st, &ev, &out));
    collect(&out);
}

static void down(ink_gesture_btn_t btn, uint32_t t)
{
    edge(btn, INK_GESTURE_LEVEL_PRESSED, t);
}

static void up(ink_gesture_btn_t btn, uint32_t t)
{
    edge(btn, INK_GESTURE_LEVEL_RELEASED, t);
}

static void expect(int index, ink_gesture_mod_t mod, ink_gesture_btn_t btn, ink_gesture_type_t type)
{
    TEST_ASSERT_LESS_THAN(s_count, index);
    TEST_ASSERT_EQUAL(mod, s_log[index].mod);
    TEST_ASSERT_EQUAL(INK_GESTURE_SRC_BTN, s_log[index].src);
    TEST_ASSERT_EQUAL(btn, s_log[index].btn);
    TEST_ASSERT_EQUAL(type, s_log[index].type);
}

void setUp(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_init(&s_st, &CFG));
    s_count = 0;
}

void tearDown(void)
{
}

/* --- Argument, config --- */

static void test_codes_are_stable(void)
{
    /* keymap lưu các số này: đổi là bảng gán đã lưu trỏ sai. */
    TEST_ASSERT_EQUAL(1, INK_GESTURE_BTN_A);
    TEST_ASSERT_EQUAL(2, INK_GESTURE_BTN_B);
    TEST_ASSERT_EQUAL(3, INK_GESTURE_BTN_AB);
    TEST_ASSERT_EQUAL(INK_GESTURE_BTN_A, INK_GESTURE_MOD_A);
    TEST_ASSERT_EQUAL(INK_GESTURE_BTN_B, INK_GESTURE_MOD_B);
    TEST_ASSERT_EQUAL(0, INK_GESTURE_TYPE_PRESS);
    TEST_ASSERT_EQUAL(4, INK_GESTURE_TYPE_HOLD_AB_LONG);
}

static void test_rejects_null_and_bad_btn(void)
{
    ink_gesture_out_t out;
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_ARG, ink_gesture_init(nullptr, &CFG));
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_ARG, ink_gesture_feed(&s_st, nullptr, &out));
    const ink_gesture_event_t ab = {.btn = INK_GESTURE_BTN_AB, .level = INK_GESTURE_LEVEL_PRESSED, .time_ms = 0};
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_ARG, ink_gesture_feed(&s_st, &ab, &out));
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_ARG, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_AB, 0));
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_ARG, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, 0x80));
}

static void test_bad_config_rejected_and_old_kept(void)
{
    ink_gesture_config_t bad = CFG;
    bad.ab_short_ms          = bad.ab_long_ms;
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_CONFIG, ink_gesture_set_config(&s_st, &bad));
    bad          = CFG;
    bad.combo_ms = bad.hold_ms;
    TEST_ASSERT_EQUAL(INK_GESTURE_ERR_CONFIG, ink_gesture_set_config(&s_st, &bad));
    ink_gesture_config_t got;
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_get_config(&s_st, &got));
    TEST_ASSERT_EQUAL(CFG.combo_ms, got.combo_ms);
    TEST_ASSERT_EQUAL(CFG.ab_short_ms, got.ab_short_ms);
}

static void test_set_config_applies_new_threshold(void)
{
    ink_gesture_config_t cfg = CFG;
    cfg.hold_ms              = 1000;
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_config(&s_st, &cfg));
    down(INK_GESTURE_BTN_A, 0);
    advance(999);
    TEST_ASSERT_EQUAL(0, s_count);
    advance(1000);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_HOLD);
}

/* --- Press, hold, debounce --- */

static void test_idle_needs_no_timer(void)
{
    uint32_t deadline;
    TEST_ASSERT_FALSE(ink_gesture_next_deadline(&s_st, &deadline));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 100);
    advance(10000);
    TEST_ASSERT_FALSE(ink_gesture_next_deadline(&s_st, &deadline));
}

static void test_press_emitted_on_release(void)
{
    down(INK_GESTURE_BTN_B, 0);
    TEST_ASSERT_EQUAL(0, s_count);
    up(INK_GESTURE_BTN_B, 100);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_B, INK_GESTURE_TYPE_PRESS);
}

static void test_wake_release_ignored(void)
{
    up(INK_GESTURE_BTN_A, 50);
    advance(5000);
    TEST_ASSERT_EQUAL(0, s_count);
}

static void test_bounce_gives_one_press(void)
{
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 3);
    down(INK_GESTURE_BTN_A, 6);
    up(INK_GESTURE_BTN_A, 100);
    down(INK_GESTURE_BTN_A, 104);
    up(INK_GESTURE_BTN_A, 108);
    advance(5000);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
}

static void test_level_after_lock_is_checked(void)
{
    /* Nhả trong debounce_ms: hết debounce mới nhận. */
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 10);
    TEST_ASSERT_EQUAL(0, s_count);
    advance(20);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
}

static void test_hold_emitted_at_threshold_not_on_release(void)
{
    down(INK_GESTURE_BTN_A, 0);
    advance(599);
    TEST_ASSERT_EQUAL(0, s_count);
    advance(600);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_HOLD);
    up(INK_GESTURE_BTN_A, 900);
    TEST_ASSERT_EQUAL(1, s_count);
}

static void test_time_wraps(void)
{
    const uint32_t t0 = UINT32_MAX - 50;
    down(INK_GESTURE_BTN_A, t0);
    up(INK_GESTURE_BTN_A, t0 + 100); /* quay vòng qua 0 */
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
    down(INK_GESTURE_BTN_B, t0 + 200);
    advance(t0 + 800);
    TEST_ASSERT_EQUAL(2, s_count);
    expect(1, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_B, INK_GESTURE_TYPE_HOLD);
}

/* --- Double press --- */

static void test_double_off_gives_two_presses(void)
{
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 50);
    down(INK_GESTURE_BTN_A, 150);
    up(INK_GESTURE_BTN_A, 200);
    TEST_ASSERT_EQUAL(2, s_count);
    expect(1, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
}

static void test_double_on_single_press_waits_window(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 50);
    advance(349);
    TEST_ASSERT_EQUAL(0, s_count);
    advance(350);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
}

static void test_double_emitted_on_second_down(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 50);
    down(INK_GESTURE_BTN_A, 150);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_DOUBLE_PRESS);
    up(INK_GESTURE_BTN_A, 200);
    advance(5000);
    TEST_ASSERT_EQUAL(1, s_count);
}

static void test_double_on_other_button_unaffected(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_B, 0);
    up(INK_GESTURE_BTN_B, 50);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_B, INK_GESTURE_TYPE_PRESS);
}

/* --- A+B --- */

static void test_ab_press(void)
{
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 50);
    up(INK_GESTURE_BTN_A, 300);
    TEST_ASSERT_EQUAL(0, s_count);
    up(INK_GESTURE_BTN_B, 400);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_AB, INK_GESTURE_TYPE_PRESS);
}

static void test_ab_hold_short_on_release(void)
{
    down(INK_GESTURE_BTN_B, 0);
    down(INK_GESTURE_BTN_A, 80);
    advance(3000);
    TEST_ASSERT_EQUAL(0, s_count);
    up(INK_GESTURE_BTN_A, 3000);
    up(INK_GESTURE_BTN_B, 3010);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_AB, INK_GESTURE_TYPE_HOLD_AB_SHORT);
}

static void test_ab_hold_long_at_threshold(void)
{
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 50);
    advance(8049);
    TEST_ASSERT_EQUAL(0, s_count);
    advance(8050);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_AB, INK_GESTURE_TYPE_HOLD_AB_LONG);
    up(INK_GESTURE_BTN_A, 9000);
    up(INK_GESTURE_BTN_B, 9000);
    TEST_ASSERT_EQUAL(1, s_count);
}

static void test_late_second_button_without_mod_ignored(void)
{
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 200);
    up(INK_GESTURE_BTN_B, 300);
    up(INK_GESTURE_BTN_A, 400);
    advance(10000);
    TEST_ASSERT_EQUAL(0, s_count);
    /* Nhả hết thì nhận lại. */
    down(INK_GESTURE_BTN_A, 11000);
    up(INK_GESTURE_BTN_A, 11100);
    TEST_ASSERT_EQUAL(1, s_count);
}

static void test_pending_press_flushed_before_combo(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 50);
    down(INK_GESTURE_BTN_B, 100);
    down(INK_GESTURE_BTN_A, 150); /* trong double_ms của A và combo_ms của B */
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
    up(INK_GESTURE_BTN_A, 300);
    up(INK_GESTURE_BTN_B, 300);
    TEST_ASSERT_EQUAL(2, s_count);
    expect(1, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_AB, INK_GESTURE_TYPE_PRESS);
}

/* --- Modifier --- */

static void test_modifier_press(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_MOD));
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 300);
    up(INK_GESTURE_BTN_B, 400);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_A, INK_GESTURE_BTN_B, INK_GESTURE_TYPE_PRESS);
    down(INK_GESTURE_BTN_B, 500);
    advance(1100);
    TEST_ASSERT_EQUAL(2, s_count);
    expect(1, INK_GESTURE_MOD_A, INK_GESTURE_BTN_B, INK_GESTURE_TYPE_HOLD);
    up(INK_GESTURE_BTN_B, 1200);
    up(INK_GESTURE_BTN_A, 1300);
    advance(5000);
    TEST_ASSERT_EQUAL(2, s_count);
}

static void test_modifier_kept_when_released_first(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_MOD));
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_B, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 300);
    up(INK_GESTURE_BTN_B, 350);
    up(INK_GESTURE_BTN_A, 400); /* nhả modifier trước khi B hết double_ms */
    advance(5000);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_A, INK_GESTURE_BTN_B, INK_GESTURE_TYPE_PRESS);
}

static void test_mod_enabled_hold_waits_release(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_MOD));
    down(INK_GESTURE_BTN_A, 0);
    advance(700);
    TEST_ASSERT_EQUAL(0, s_count);
    up(INK_GESTURE_BTN_A, 800);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_HOLD);
}

static void test_mod_enabled_short_press_still_press(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_MOD));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 200);
    TEST_ASSERT_EQUAL(1, s_count);
    expect(0, INK_GESTURE_MOD_NONE, INK_GESTURE_BTN_A, INK_GESTURE_TYPE_PRESS);
}

/* --- Đổi mask giữa chừng --- */

static void test_mask_change_drops_pending_after_release(void)
{
    /* Ví dụ trong docs: A đang chờ double press thì scene đổi, tắt double. */
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 50);
    advance(100);
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_NONE));
    advance(5000);
    TEST_ASSERT_EQUAL(0, s_count);
}

static void test_mask_change_drops_pending_while_held(void)
{
    down(INK_GESTURE_BTN_A, 0);
    advance(100);
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    advance(1000);
    up(INK_GESTURE_BTN_A, 1100);
    advance(5000);
    TEST_ASSERT_EQUAL(0, s_count);
    down(INK_GESTURE_BTN_A, 6000);
    up(INK_GESTURE_BTN_A, 6100);
    advance(7000);
    TEST_ASSERT_EQUAL(1, s_count);
}

static void test_same_mask_keeps_pending(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    down(INK_GESTURE_BTN_A, 0);
    up(INK_GESTURE_BTN_A, 50);
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_DOUBLE));
    advance(5000);
    TEST_ASSERT_EQUAL(1, s_count);
}

/* --- Progress --- */

static ink_gesture_progress_t progress_at(uint32_t now)
{
    advance(now);
    ink_gesture_progress_t p;
    TEST_ASSERT_TRUE(ink_gesture_progress(&s_st, now, &p));
    return p;
}

static void expect_no_progress(uint32_t now)
{
    advance(now);
    ink_gesture_progress_t p;
    TEST_ASSERT_FALSE(ink_gesture_progress(&s_st, now, &p));
}

static void test_progress_none_when_idle(void)
{
    ink_gesture_progress_t p;
    TEST_ASSERT_FALSE(ink_gesture_progress(nullptr, 0, &p));
    TEST_ASSERT_FALSE(ink_gesture_progress(&s_st, 0, nullptr));
    expect_no_progress(0);
}

static void test_progress_hold(void)
{
    down(INK_GESTURE_BTN_A, 0);
    const ink_gesture_progress_t p = progress_at(100);
    TEST_ASSERT_EQUAL(INK_GESTURE_MOD_NONE, p.mod);
    TEST_ASSERT_EQUAL(INK_GESTURE_BTN_A, p.btn);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_PRESS, p.on_release);
    TEST_ASSERT_TRUE(p.has_next);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_HOLD, p.next);
    TEST_ASSERT_EQUAL(0, p.start_ms);
    TEST_ASSERT_EQUAL(600, p.next_ms);
    expect_no_progress(700); /* HOLD đã phát */
}

static void test_progress_mod_hold_waits_release(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_MOD));
    down(INK_GESTURE_BTN_A, 0);
    const ink_gesture_progress_t p = progress_at(700);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_HOLD, p.on_release);
    TEST_ASSERT_FALSE(p.has_next);
}

static void test_progress_ab_short_then_long(void)
{
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 50);
    ink_gesture_progress_t p = progress_at(100);
    TEST_ASSERT_EQUAL(INK_GESTURE_BTN_AB, p.btn);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_PRESS, p.on_release);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_HOLD_AB_SHORT, p.next);
    TEST_ASSERT_EQUAL(50, p.start_ms);
    TEST_ASSERT_EQUAL(2050, p.next_ms);
    p = progress_at(3000);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_HOLD_AB_SHORT, p.on_release);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_HOLD_AB_LONG, p.next);
    TEST_ASSERT_EQUAL(8050, p.next_ms);
    expect_no_progress(8050); /* HOLD_AB_LONG đã phát */
}

static void test_progress_modifier_target(void)
{
    TEST_ASSERT_EQUAL(INK_GESTURE_OK, ink_gesture_set_enabled(&s_st, INK_GESTURE_BTN_A, INK_GESTURE_WAIT_MOD));
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 300);
    const ink_gesture_progress_t p = progress_at(400);
    TEST_ASSERT_EQUAL(INK_GESTURE_MOD_A, p.mod);
    TEST_ASSERT_EQUAL(INK_GESTURE_BTN_B, p.btn);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_PRESS, p.on_release);
    TEST_ASSERT_EQUAL(INK_GESTURE_TYPE_HOLD, p.next);
    TEST_ASSERT_EQUAL(900, p.next_ms);
}

static void test_progress_none_when_blocked(void)
{
    down(INK_GESTURE_BTN_A, 0);
    down(INK_GESTURE_BTN_B, 200);
    expect_no_progress(300);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_codes_are_stable);
    RUN_TEST(test_rejects_null_and_bad_btn);
    RUN_TEST(test_bad_config_rejected_and_old_kept);
    RUN_TEST(test_set_config_applies_new_threshold);
    RUN_TEST(test_idle_needs_no_timer);
    RUN_TEST(test_press_emitted_on_release);
    RUN_TEST(test_wake_release_ignored);
    RUN_TEST(test_bounce_gives_one_press);
    RUN_TEST(test_level_after_lock_is_checked);
    RUN_TEST(test_hold_emitted_at_threshold_not_on_release);
    RUN_TEST(test_time_wraps);
    RUN_TEST(test_double_off_gives_two_presses);
    RUN_TEST(test_double_on_single_press_waits_window);
    RUN_TEST(test_double_emitted_on_second_down);
    RUN_TEST(test_double_on_other_button_unaffected);
    RUN_TEST(test_ab_press);
    RUN_TEST(test_ab_hold_short_on_release);
    RUN_TEST(test_ab_hold_long_at_threshold);
    RUN_TEST(test_late_second_button_without_mod_ignored);
    RUN_TEST(test_pending_press_flushed_before_combo);
    RUN_TEST(test_modifier_press);
    RUN_TEST(test_modifier_kept_when_released_first);
    RUN_TEST(test_mod_enabled_hold_waits_release);
    RUN_TEST(test_mod_enabled_short_press_still_press);
    RUN_TEST(test_mask_change_drops_pending_after_release);
    RUN_TEST(test_mask_change_drops_pending_while_held);
    RUN_TEST(test_same_mask_keeps_pending);
    RUN_TEST(test_progress_none_when_idle);
    RUN_TEST(test_progress_hold);
    RUN_TEST(test_progress_mod_hold_waits_release);
    RUN_TEST(test_progress_ab_short_then_long);
    RUN_TEST(test_progress_modifier_target);
    RUN_TEST(test_progress_none_when_blocked);
    return UNITY_END();
}
