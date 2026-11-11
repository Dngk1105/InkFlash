/* Design: docs/design/display/display.md */

/* Host test: runs on the PC with gcc and the Unity copy from ESP-IDF. One test_* function per case. */
#include "ink_display_policy.h"
#include "unity.h"
#include <string.h>

/* Small 20 × 8 panel: 3 bytes per row, 4 padding bits at row end. */
#define WIDTH     20
#define HEIGHT    8
#define ROW_BYTES 3
#define SIZE      (ROW_BYTES * HEIGHT)

static uint8_t                    s_shown[SIZE];
static uint8_t                    s_next[SIZE];
static uint8_t                    s_old[SIZE];
static uint8_t                    s_pixels[WIDTH * HEIGHT];
static ink_display_policy_state_t s_state;
static uint32_t                   s_now_ms;

static ink_display_policy_config_t default_config(void) {
    return (ink_display_policy_config_t){
        .width            = WIDTH,
        .height           = HEIGHT,
        .x_align          = 8,
        .debt_weights     = {[INK_DISPLAY_INTENT_TEXT] = 1,    [INK_DISPLAY_INTENT_UI] = 2,    [INK_DISPLAY_INTENT_PAGE] = 1   },
        .marks_edges      = {[INK_DISPLAY_INTENT_TEXT] = true, [INK_DISPLAY_INTENT_UI] = true, [INK_DISPLAY_INTENT_PAGE] = true},
        .auto_ui_percent  = 10,
        .debt_light       = 2,
        .debt_page        = 6,
        .debt_ceiling     = 8,
        .debt_ceiling_max = 15,
        .active_ms        = 2000,
        .edge_ms          = 2000,
        .light_ms         = 10000,
        .strong_ms        = 60000,
        .fast_limit       = 4,
    };
}

static void set_black(uint8_t *frame, uint32_t x, uint32_t y) {
    frame[y * ROW_BYTES + x / 8] &= (uint8_t)~(0x80U >> (x % 8));
}

static void set_white(uint8_t *frame, uint32_t x, uint32_t y) {
    frame[y * ROW_BYTES + x / 8] |= (uint8_t)(0x80U >> (x % 8));
}

static bool is_white(const uint8_t *frame, uint32_t x, uint32_t y) {
    return (frame[y * ROW_BYTES + x / 8] & (0x80U >> (x % 8))) != 0;
}

static ink_display_policy_request_t request(ink_display_intent_t intent, uint8_t flags) {
    return (ink_display_policy_request_t){.intent = intent, .flags = flags};
}

static void expect_decision_cleanup(ink_display_cleanup_t cleanup, ink_display_policy_decision_t decision) {
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_CLEANUP, decision.action);
    TEST_ASSERT_EQUAL(cleanup, decision.cleanup);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
}

static void expect_idle_cleanup(ink_display_cleanup_t cleanup, ink_display_policy_idle_t idle) {
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_CLEANUP, idle.action);
    TEST_ASSERT_EQUAL(cleanup, idle.cleanup);
}

/* decide s_next, then treat s_next as the shown frame. */
static ink_display_policy_decision_t draw_next(ink_display_intent_t intent, uint8_t flags) {
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_decide(&s_state, s_shown, s_next, request(intent, flags), s_now_ms, &decision));
    memcpy(s_shown, s_next, SIZE);
    return decision;
}

/* Init and draw the first frame (cleanup strong), so s_shown is the shown frame. */
static void start(const ink_display_policy_config_t *config) {
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, config, s_pixels));
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_decide(&s_state, nullptr, s_shown, request(INK_DISPLAY_INTENT_TEXT, 0), s_now_ms, &decision));
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, decision);
}

static ink_display_policy_idle_t idle_at(uint32_t now_ms) {
    ink_display_policy_idle_t idle;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_idle(&s_state, now_ms, &idle));
    return idle;
}

/* Black 2 × 2 box at (x, y) on a white s_next. */
static void black_box(uint32_t x, uint32_t y) {
    memset(s_next, 0xFF, SIZE);
    set_black(s_next, x, y);
    set_black(s_next, x + 1, y);
    set_black(s_next, x, y + 1);
    set_black(s_next, x + 1, y + 1);
}

void setUp(void) {
    memset(s_shown, 0xFF, SIZE);
    memset(s_next, 0xFF, SIZE);
    memset(s_old, 0xAA, SIZE);
    memset(s_pixels, 0xEE, sizeof(s_pixels));
    s_now_ms = 1000;
}

void tearDown(void) {
}

/* ---- Init, config ---- */

static void test_init_rejects_bad_config(void) {
    ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(nullptr, &config, s_pixels));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&s_state, nullptr, s_pixels));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&s_state, &config, nullptr));
    config.x_align = 0;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&s_state, &config, s_pixels));
    config                  = default_config();
    config.debt_ceiling     = 9;
    config.debt_ceiling_max = 8;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&s_state, &config, s_pixels));
    config                  = default_config();
    config.debt_ceiling_max = INK_DISPLAY_POLICY_MAX_DEBT + 1;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&s_state, &config, s_pixels));
    config                 = default_config();
    config.auto_ui_percent = 101;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_init(&s_state, &config, s_pixels));
}

static void test_init_clears_pixels(void) {
    const ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, &config, s_pixels));
    for (size_t i = 0; i < sizeof(s_pixels); i++) { TEST_ASSERT_EQUAL_UINT8(0, s_pixels[i]); }
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
    TEST_ASSERT_EQUAL_UINT32(0, ink_display_policy_flagged_count(&s_state));
}

static void test_set_config_rejects_size_change(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_config_t changed = config;
    changed.width                       = WIDTH - 1;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_set_config(&s_state, &changed));
    changed            = config;
    changed.debt_light = 3;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_set_config(&s_state, &changed));
}

static void test_decide_rejects_bad_arguments(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_decide(&s_state, nullptr, s_next, request(INK_DISPLAY_INTENT_TEXT, 0), 0, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_decide(&s_state, s_shown, s_next, request(INK_DISPLAY_INTENT_COUNT, 0), 0, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_decide(&s_state, s_shown, s_next, request(INK_DISPLAY_INTENT_TEXT, 0), 0, nullptr));
}

/* ---- Decide ---- */

static void test_first_decide_is_cleanup_strong_with_inverted_plane(void) {
    const ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, &config, s_pixels));
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_decide(&s_state, nullptr, s_next, request(INK_DISPLAY_INTENT_UI, 0), 0, &decision));
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, decision);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(WIDTH, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(HEIGHT, decision.window.height);
}

static void test_identical_frame_draws_nothing(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, decision.action);
    TEST_ASSERT_EQUAL_UINT16(0, decision.window.width);
}

static void test_draw_uses_changed_window_and_shown_plane(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 10, 3);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_INTENT_TEXT, decision.intent);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SHOWN, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(8, decision.window.x); /* aligned to x_align 8 */
    TEST_ASSERT_EQUAL_UINT16(8, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(3, decision.window.y);
    TEST_ASSERT_EQUAL_UINT16(1, decision.window.height);
}

static void test_window_is_clipped_to_screen_width(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 19, 0);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL_UINT16(16, decision.window.x);
    TEST_ASSERT_EQUAL_UINT16(4, decision.window.width);
}

static void test_auto_picks_ui_for_small_window_and_text_for_large(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 2, 2); /* window 8 × 1 = 5 % of the panel */
    TEST_ASSERT_EQUAL(INK_DISPLAY_INTENT_UI, draw_next(INK_DISPLAY_INTENT_AUTO, 0).intent);
    memset(s_next, 0x00, SIZE); /* whole panel changes */
    TEST_ASSERT_EQUAL(INK_DISPLAY_INTENT_TEXT, draw_next(INK_DISPLAY_INTENT_AUTO, 0).intent);
}

static void test_after_wake_draws_whole_screen_with_intent(void) {
    const ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, &config, s_pixels));
    ink_display_policy_assume_shown(&s_state);
    set_black(s_next, 1, 1);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_PAGE, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_INTENT_PAGE, decision.intent);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SHOWN, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(WIDTH, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(HEIGHT, decision.window.height);
    set_black(s_next, 12, 1);
    TEST_ASSERT_EQUAL_UINT16(8, draw_next(INK_DISPLAY_INTENT_PAGE, 0).window.width);
}

static void test_invalidate_forces_cleanup_strong(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_invalidate(&s_state);
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_decide(&s_state, nullptr, s_next, request(INK_DISPLAY_INTENT_TEXT, 0), s_now_ms, &decision));
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, decision);
}

/* ---- Debt ---- */

static void test_debt_adds_weight_on_changed_pixels_only(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(2, ink_display_policy_max_debt(&s_state));
    set_white(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL_UINT8(3, ink_display_policy_max_debt(&s_state));
    TEST_ASSERT_EQUAL_UINT8(3 << 2, s_pixels[4 * WIDTH + 4] & 0x3C);
    TEST_ASSERT_EQUAL_UINT8(0, s_pixels[4 * WIDTH + 5] & 0x3C); /* inside window but unchanged */
}

static void test_exceeding_ceiling_draws_cleanup_strong_and_clears_debt(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    /* long idle between flips: ceiling stays 8; UI adds 2 each time */
    for (int i = 0; i < 4; i++) {
        s_now_ms += 5000;
        if (i % 2 == 0) {
            set_black(s_next, 4, 4);
        } else {
            set_white(s_next, 4, 4);
        }
        TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, draw_next(INK_DISPLAY_INTENT_UI, 0).action);
    }
    TEST_ASSERT_EQUAL_UINT8(8, ink_display_policy_max_debt(&s_state));
    s_now_ms += 5000;
    set_black(s_next, 4, 4);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_UI, 0);
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, decision);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
}

/* Flip (4, 4) with UI until the ceiling of 8 is reached during an active period; s_shown has the next frame. */
static ink_display_policy_decision_t flip_cell_past_ceiling(void) {
    for (int i = 0; i < 4; i++) {
        s_now_ms += 100;
        if (i % 2 == 0) {
            set_black(s_next, 4, 4);
        } else {
            set_white(s_next, 4, 4);
        }
        TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, draw_next(INK_DISPLAY_INTENT_UI, 0).action);
    }
    s_now_ms += 100;
    set_black(s_next, 4, 4);
    return draw_next(INK_DISPLAY_INTENT_UI, 0);
}

static void test_exceeding_ceiling_while_active_draws_cleanup_fast(void) {
    ink_display_policy_config_t config = default_config();
    config.debt_ceiling_max            = config.debt_ceiling; /* ceiling does not rise */
    start(&config);
    const ink_display_policy_decision_t decision = flip_cell_past_ceiling();
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_CLEANUP, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_FAST, decision.cleanup);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SELECTED, decision.old_plane);
}

static void test_cleanup_fast_drives_dirty_pixels_only(void) {
    ink_display_policy_config_t config = default_config();
    config.debt_ceiling_max            = config.debt_ceiling;
    set_black(s_shown, 16, 6); /* text far from the cell: clean after the first cleanup strong */
    set_black(s_next, 16, 6);
    start(&config);
    (void)flip_cell_past_ceiling();

    const uint32_t     flagged_before = ink_display_policy_flagged_count(&s_state);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_FAST, s_old, &window));
    TEST_ASSERT_NOT_EQUAL(is_white(s_shown, 4, 4), is_white(s_old, 4, 4));
    TEST_ASSERT_EQUAL(is_white(s_shown, 16, 6), is_white(s_old, 16, 6));
    TEST_ASSERT_EQUAL_UINT16(0, window.x);
    TEST_ASSERT_EQUAL_UINT16(8, window.width);

    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_FAST));
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
    /* edge flags in the rows above and below the cell lie outside the changed window: left for the idle edge stage */
    TEST_ASSERT_EQUAL_UINT32(flagged_before, ink_display_policy_flagged_count(&s_state));
}

static void test_cleanup_fast_leaves_dirty_pixels_outside_window(void) {
    ink_display_policy_config_t config = default_config();
    config.debt_ceiling_max            = config.debt_ceiling;
    start(&config);
    s_now_ms += 5000;
    set_black(s_next, 16, 6); /* text drawn with TEXT: debt 1, far from the cell */
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, draw_next(INK_DISPLAY_INTENT_TEXT, 0).action);
    (void)flip_cell_past_ceiling();

    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_FAST, s_old, &window));
    TEST_ASSERT_NOT_EQUAL(is_white(s_shown, 4, 4), is_white(s_old, 4, 4));
    TEST_ASSERT_EQUAL(is_white(s_shown, 16, 6), is_white(s_old, 16, 6));
    TEST_ASSERT_EQUAL_UINT16(0, window.x);
    TEST_ASSERT_EQUAL_UINT16(8, window.width);

    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_FAST));
    TEST_ASSERT_EQUAL_UINT8(1, ink_display_policy_max_debt(&s_state));
}

static void test_ceiling_grows_while_active_and_resets_after_rest(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    for (int i = 0; i < 7; i++) {
        s_now_ms += 100;
        if (i % 2 == 0) {
            set_black(s_next, 4, 4);
        } else {
            set_white(s_next, 4, 4);
        }
        TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, draw_next(INK_DISPLAY_INTENT_UI, 0).action);
    }
    TEST_ASSERT_EQUAL_UINT8(14, ink_display_policy_max_debt(&s_state));
    TEST_ASSERT_EQUAL_UINT8(15, ink_display_policy_ceiling(&s_state));
    s_now_ms += 5000; /* rest longer than active_ms: ceiling back to 8; debt 14 + 2 exceeds it */
    set_white(s_next, 4, 4);
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, draw_next(INK_DISPLAY_INTENT_UI, 0));
    TEST_ASSERT_EQUAL_UINT8(8, ink_display_policy_ceiling(&s_state));
}

static void test_page_with_high_debt_draws_cleanup_strong(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    for (int i = 0; i < 3; i++) {
        s_now_ms += 100;
        if (i % 2 == 0) {
            set_black(s_next, 4, 4);
        } else {
            set_white(s_next, 4, 4);
        }
        draw_next(INK_DISPLAY_INTENT_UI, 0);
    }
    TEST_ASSERT_EQUAL_UINT8(6, ink_display_policy_max_debt(&s_state));
    set_black(s_next, 15, 6);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_PAGE, 0);
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, decision);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
}

static void test_before_sleep_cleans_only_when_dirty(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 2, 2); /* clean panel: normal draw */
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, draw_next(INK_DISPLAY_INTENT_PAGE, INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP).action);
    set_black(s_next, 12, 2); /* debt left from the previous frame */
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, draw_next(INK_DISPLAY_INTENT_PAGE, INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP));
}

/* ---- REDRAW_WINDOW ---- */

/* Shown frame: black at (2,2) (3,2) and far away (16,6). Next frame adds black (5,2). Returns the decision of next. */
static ink_display_policy_decision_t redraw_after_adding_black(uint8_t flags) {
    const ink_display_policy_config_t config = default_config();
    set_black(s_shown, 2, 2);
    set_black(s_shown, 3, 2);
    set_black(s_shown, 16, 6);
    start(&config);
    memcpy(s_next, s_shown, SIZE);
    set_black(s_next, 5, 2);
    s_now_ms += 100;
    return draw_next(INK_DISPLAY_INTENT_UI, flags);
}

static void test_redraw_window_draws_cleanup_fast_with_selected_plane(void) {
    const ink_display_policy_decision_t decision = redraw_after_adding_black(INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_CLEANUP, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_FAST, decision.cleanup);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SELECTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(0, decision.window.x);
    TEST_ASSERT_EQUAL_UINT16(8, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(2, decision.window.y);
    TEST_ASSERT_EQUAL_UINT16(1, decision.window.height);
}

static void test_redraw_window_inverts_unchanged_pixels_inside_window_only(void) {
    (void)redraw_after_adding_black(INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_FAST, s_old, &window));
    TEST_ASSERT_NOT_EQUAL(is_white(s_shown, 2, 2), is_white(s_old, 2, 2)); /* black -> black, unchanged */
    TEST_ASSERT_NOT_EQUAL(is_white(s_shown, 3, 2), is_white(s_old, 3, 2));
    TEST_ASSERT_NOT_EQUAL(is_white(s_shown, 7, 2), is_white(s_old, 7, 2)); /* white -> white, in window, outside changed bounds */
    TEST_ASSERT_NOT_EQUAL(is_white(s_shown, 5, 2), is_white(s_old, 5, 2)); /* changed pixel */
    TEST_ASSERT_EQUAL(is_white(s_shown, 16, 6), is_white(s_old, 16, 6));   /* outside window */
    TEST_ASSERT_EQUAL(is_white(s_shown, 2, 3), is_white(s_old, 2, 3));     /* row outside window */
    TEST_ASSERT_EQUAL_UINT16(8, window.width);
}

static void test_redraw_window_debt_is_cleared_by_cleanup_done(void) {
    (void)redraw_after_adding_black(INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW);
    TEST_ASSERT_EQUAL_UINT8(2, ink_display_policy_max_debt(&s_state)); /* UI weight 2 on every pixel of the window */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_FAST));
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
    TEST_ASSERT_EQUAL_UINT32(0, ink_display_policy_flagged_count(&s_state));
}

/* Toggle pixel (5,2) with REDRAW_WINDOW: one cleanup fast decision per call (cleanup_done included). */
static ink_display_policy_decision_t redraw_toggle(void) {
    if (is_white(s_shown, 5, 2)) {
        set_black(s_next, 5, 2);
    } else {
        set_white(s_next, 5, 2);
    }
    s_now_ms += 100;
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_UI, INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW);
    if (decision.cleanup == INK_DISPLAY_CLEANUP_FAST) {
        TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_FAST));
    }
    return decision;
}

static void test_cleanup_fast_after_fast_limit_is_cleanup_strong(void) {
    (void)redraw_after_adding_black(INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW); /* cleanup fast 1 */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_FAST));
    for (int i = 2; i <= 4; i++) { TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_FAST, redraw_toggle().cleanup); }
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG, redraw_toggle());
    TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_FAST, redraw_toggle().cleanup); /* cleanup strong reset the count */
}

static void test_zero_fast_limit_never_turns_cleanup_fast_into_strong(void) {
    ink_display_policy_config_t config = default_config();
    config.fast_limit                  = 0;
    set_black(s_shown, 2, 2);
    start(&config);
    memcpy(s_next, s_shown, SIZE);
    for (int i = 0; i < 20; i++) { TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_FAST, redraw_toggle().cleanup); }
}

static void test_idle_cleanup_strong_resets_fast_count(void) {
    (void)redraw_after_adding_black(INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW); /* cleanup fast 1 */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_FAST));
    for (int i = 2; i <= 4; i++) { (void)redraw_toggle(); }
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_STRONG));
    TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_FAST, redraw_toggle().cleanup);
}

static void test_redraw_window_not_requested_draws_changed_pixels_only(void) {
    const ink_display_policy_decision_t decision = redraw_after_adding_black(0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SHOWN, decision.old_plane);
}

static void test_before_sleep_beats_redraw_window_when_dirty(void) {
    const ink_display_policy_decision_t first = redraw_after_adding_black(0); /* leaves debt */
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, first.action);
    set_black(s_next, 6, 2);
    s_now_ms += 100;
    expect_decision_cleanup(INK_DISPLAY_CLEANUP_STRONG,
                            draw_next(INK_DISPLAY_INTENT_UI, INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW | INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP));
}

static void test_redraw_window_with_identical_frame_draws_nothing(void) {
    (void)redraw_after_adding_black(0);
    s_now_ms += 100;
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_UI, INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, decision.action);
}

static void test_redraw_window_after_wake_draws_whole_screen_with_intent(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_assume_shown(&s_state);
    set_black(s_next, 5, 2);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_UI, INK_DISPLAY_POLICY_FLAG_REDRAW_WINDOW);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL_UINT16(WIDTH, decision.window.width);
}

/* ---- Edge flags ---- */

static void test_marks_dark_edge_around_box_that_turns_white(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    /* row order: (4, 2) is next to (4, 3), which just left black -> dark edge flag; (3, 3) has (4, 2) diagonal -> no flag (no clusters) */
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[2 * WIDTH + 4] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(0, s_pixels[3 * WIDTH + 3] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[3 * WIDTH + 6] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[4 * WIDTH + 3] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[5 * WIDTH + 5] & 0x03);
    TEST_ASSERT_EQUAL_UINT32(4, ink_display_policy_flagged_count(&s_state));
}

static void test_intent_without_edge_flags_marks_nothing_but_adds_debt(void) {
    ink_display_policy_config_t config          = default_config();
    config.marks_edges[INK_DISPLAY_INTENT_TEXT] = false;
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL_UINT32(0, ink_display_policy_flagged_count(&s_state));
    TEST_ASSERT_EQUAL_UINT8(1, ink_display_policy_max_debt(&s_state));
    black_box(4, 3);
    s_now_ms += 100;
    draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL_UINT32(0, ink_display_policy_flagged_count(&s_state));
    /* UI still marks edges: box back to white as in the test above */
    memset(s_next, 0xFF, SIZE);
    s_now_ms += 100;
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT32(4, ink_display_policy_flagged_count(&s_state));
}

static void test_marks_light_edge_next_to_pixel_that_turns_black(void) {
    const ink_display_policy_config_t config = default_config();
    set_black(s_shown, 5, 5);
    start(&config);
    memcpy(s_next, s_shown, SIZE);
    set_black(s_next, 6, 5);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(2, s_pixels[5 * WIDTH + 5] & 0x03);
}

static void test_changed_pixel_loses_its_flag(void) {
    const ink_display_policy_config_t config = default_config();
    set_black(s_shown, 5, 5);
    start(&config);
    memcpy(s_next, s_shown, SIZE);
    set_black(s_next, 6, 5);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    const uint32_t flagged = ink_display_policy_flagged_count(&s_state);
    set_white(s_next, 5, 5);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(0, s_pixels[5 * WIDTH + 5] & 0x03);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(flagged + 4, ink_display_policy_flagged_count(&s_state));
}

/* ---- Cleanup stages ---- */

static void test_idle_ladder_runs_edge_then_light_then_strong(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    const uint32_t drawn_ms = s_now_ms;

    ink_display_policy_idle_t idle = idle_at(drawn_ms + 500);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, idle.action);
    TEST_ASSERT_EQUAL_UINT32(1500, idle.wait_ms);

    expect_idle_cleanup(INK_DISPLAY_CLEANUP_EDGE_CENTER, idle_at(drawn_ms + 2000));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_EDGE_CENTER));
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR, idle_at(drawn_ms + 2100));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR));
    TEST_ASSERT_EQUAL_UINT32(0, ink_display_policy_flagged_count(&s_state));

    idle = idle_at(drawn_ms + 2200);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, idle.action);
    TEST_ASSERT_EQUAL_UINT32(7800, idle.wait_ms);
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_LIGHT, idle_at(drawn_ms + 10000));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_LIGHT));
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));

    /* no debt and no edge flag: no stage left */
    idle = idle_at(drawn_ms + 70000);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, idle.action);
    TEST_ASSERT_EQUAL_UINT32(INK_DISPLAY_POLICY_WAIT_FOREVER, idle.wait_ms);
}

static void test_idle_strong_runs_when_debt_below_soft_remains(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_TEXT, 0); /* debt 1 < debt_light */
    const uint32_t drawn_ms = s_now_ms;
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, idle_at(drawn_ms + 30000).action);
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_STRONG, idle_at(drawn_ms + 60000));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_STRONG));
    TEST_ASSERT_EQUAL_UINT32(INK_DISPLAY_POLICY_WAIT_FOREVER, idle_at(drawn_ms + 60001).wait_ms);
}

static void test_new_frame_restarts_ladder(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_EDGE_CENTER));
    s_now_ms += 2100;
    set_black(s_next, 15, 0);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    /* edge center cleanup runs again: stages restart from the new frame */
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, idle_at(s_now_ms + 1000).action);
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_EDGE_CENTER, idle_at(s_now_ms + 2000));
}

static void test_defer_flag_blocks_ladder_until_next_frame(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_NONE, idle_at(s_now_ms + 100000).action);
    TEST_ASSERT_EQUAL_UINT32(INK_DISPLAY_POLICY_WAIT_FOREVER, idle_at(s_now_ms + 100000).wait_ms);
    set_white(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_EDGE_CENTER, idle_at(s_now_ms + 2000));
}

/* active_ms and edge_ms are independent: no cleanup stage 1, ceiling still grows. */
static void test_active_period_without_edge_stage(void) {
    ink_display_policy_config_t config = default_config();
    config.edge_ms                     = 0;
    start(&config);
    s_now_ms += 100;
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    s_now_ms += 100;
    set_white(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    /* start() draws the first frame, so both frames above are in the active period */
    TEST_ASSERT_EQUAL_UINT8(config.debt_ceiling + 2, ink_display_policy_ceiling(&s_state));
}

/* active_ms = 0: frames close together do not raise the ceiling. */
static void test_zero_active_ms_keeps_ceiling(void) {
    ink_display_policy_config_t config = default_config();
    config.active_ms                   = 0;
    start(&config);
    s_now_ms += 100;
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    s_now_ms += 100;
    set_white(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(config.debt_ceiling, ink_display_policy_ceiling(&s_state));
}

static void test_zero_stage_time_skips_stage(void) {
    ink_display_policy_config_t config = default_config();
    config.edge_ms                     = 0;
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_LIGHT, idle_at(s_now_ms + 10000));
}

static void test_idle_handles_clock_wrap(void) {
    const ink_display_policy_config_t config = default_config();
    s_now_ms                                 = UINT32_MAX - 500;
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    expect_idle_cleanup(INK_DISPLAY_CLEANUP_LIGHT, idle_at(s_now_ms + 10000));
}

/* ---- Old plane of a cleanup ---- */

static void test_cleanup_rejects_invalid_cleanup(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_INVALID, s_old, &window));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_COUNT, s_old, &window));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_INVALID));
}

static void test_draw_decision_has_no_cleanup(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 2, 2);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_CLEANUP_INVALID, decision.cleanup);
}

static void test_cleanup_plane_inverts_flagged_centers_only(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_EDGE_CENTER, s_old, &window));
    uint32_t inverted = 0;
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) {
            const bool flipped = is_white(s_old, x, y) != is_white(s_shown, x, y);
            TEST_ASSERT_EQUAL((s_pixels[y * WIDTH + x] & 0x03) != 0, flipped);
            if (flipped) { inverted++; }
        }
    }
    TEST_ASSERT_EQUAL_UINT32(ink_display_policy_flagged_count(&s_state), inverted);
    TEST_ASSERT_GREATER_THAN_UINT16(0, window.width);
    TEST_ASSERT_EQUAL_UINT16(0, window.x % 8);
}

static void test_cleanup_plane_neighbor_inverts_cardinal_neighbors_of_centers(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_INTENT_UI, 0);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_EDGE_NEIGHBOR, s_old, &window));
    /* (3, 4) is the center: not inverted; (3, 3) right above, white, no flag: inverted */
    TEST_ASSERT_TRUE(is_white(s_old, 3, 4));
    TEST_ASSERT_FALSE(is_white(s_old, 3, 3));
}

static void test_cleanup_plane_light_inverts_pixels_with_soft_debt(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    set_black(s_next, 6, 4);
    draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    s_now_ms += 100;
    set_white(s_next, 4, 4); /* (4, 4) debt 2, (6, 4) debt 1 */
    draw_next(INK_DISPLAY_INTENT_TEXT, 0);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_LIGHT, s_old, &window));
    TEST_ASSERT_FALSE(is_white(s_old, 4, 4));
    TEST_ASSERT_FALSE(is_white(s_old, 6, 4)); /* black, not inverted */
    TEST_ASSERT_EQUAL_UINT16(0, window.x);
    TEST_ASSERT_EQUAL_UINT16(8, window.width);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_CLEANUP_LIGHT));
    TEST_ASSERT_EQUAL_UINT8(1, ink_display_policy_max_debt(&s_state));
}

static void test_cleanup_plane_strong_inverts_whole_screen(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_CLEANUP_STRONG, s_old, &window));
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) { TEST_ASSERT_NOT_EQUAL(is_white(s_shown, x, y), is_white(s_old, x, y)); }
    }
    TEST_ASSERT_EQUAL_UINT16(WIDTH, window.width);
    TEST_ASSERT_EQUAL_UINT16(HEIGHT, window.height);
}

/* ---- Changed window ---- */

static void test_changed_window_helper(void) {
    const ink_display_policy_config_t config = default_config();
    ink_display_rect_t                window;
    TEST_ASSERT_FALSE(ink_display_policy_changed_window(&config, s_shown, s_next, &window));
    set_black(s_next, 9, 2);
    set_black(s_next, 17, 5);
    TEST_ASSERT_TRUE(ink_display_policy_changed_window(&config, s_shown, s_next, &window));
    TEST_ASSERT_EQUAL_UINT16(8, window.x);
    TEST_ASSERT_EQUAL_UINT16(12, window.width);
    TEST_ASSERT_EQUAL_UINT16(2, window.y);
    TEST_ASSERT_EQUAL_UINT16(4, window.height);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_config);
    RUN_TEST(test_init_clears_pixels);
    RUN_TEST(test_set_config_rejects_size_change);
    RUN_TEST(test_decide_rejects_bad_arguments);
    RUN_TEST(test_first_decide_is_cleanup_strong_with_inverted_plane);
    RUN_TEST(test_identical_frame_draws_nothing);
    RUN_TEST(test_draw_uses_changed_window_and_shown_plane);
    RUN_TEST(test_window_is_clipped_to_screen_width);
    RUN_TEST(test_auto_picks_ui_for_small_window_and_text_for_large);
    RUN_TEST(test_after_wake_draws_whole_screen_with_intent);
    RUN_TEST(test_invalidate_forces_cleanup_strong);
    RUN_TEST(test_debt_adds_weight_on_changed_pixels_only);
    RUN_TEST(test_exceeding_ceiling_draws_cleanup_strong_and_clears_debt);
    RUN_TEST(test_exceeding_ceiling_while_active_draws_cleanup_fast);
    RUN_TEST(test_cleanup_fast_drives_dirty_pixels_only);
    RUN_TEST(test_cleanup_fast_leaves_dirty_pixels_outside_window);
    RUN_TEST(test_ceiling_grows_while_active_and_resets_after_rest);
    RUN_TEST(test_page_with_high_debt_draws_cleanup_strong);
    RUN_TEST(test_before_sleep_cleans_only_when_dirty);
    RUN_TEST(test_redraw_window_draws_cleanup_fast_with_selected_plane);
    RUN_TEST(test_redraw_window_inverts_unchanged_pixels_inside_window_only);
    RUN_TEST(test_redraw_window_debt_is_cleared_by_cleanup_done);
    RUN_TEST(test_redraw_window_not_requested_draws_changed_pixels_only);
    RUN_TEST(test_before_sleep_beats_redraw_window_when_dirty);
    RUN_TEST(test_redraw_window_with_identical_frame_draws_nothing);
    RUN_TEST(test_redraw_window_after_wake_draws_whole_screen_with_intent);
    RUN_TEST(test_cleanup_fast_after_fast_limit_is_cleanup_strong);
    RUN_TEST(test_zero_fast_limit_never_turns_cleanup_fast_into_strong);
    RUN_TEST(test_idle_cleanup_strong_resets_fast_count);
    RUN_TEST(test_marks_dark_edge_around_box_that_turns_white);
    RUN_TEST(test_intent_without_edge_flags_marks_nothing_but_adds_debt);
    RUN_TEST(test_marks_light_edge_next_to_pixel_that_turns_black);
    RUN_TEST(test_changed_pixel_loses_its_flag);
    RUN_TEST(test_idle_ladder_runs_edge_then_light_then_strong);
    RUN_TEST(test_idle_strong_runs_when_debt_below_soft_remains);
    RUN_TEST(test_new_frame_restarts_ladder);
    RUN_TEST(test_defer_flag_blocks_ladder_until_next_frame);
    RUN_TEST(test_zero_stage_time_skips_stage);
    RUN_TEST(test_active_period_without_edge_stage);
    RUN_TEST(test_zero_active_ms_keeps_ceiling);
    RUN_TEST(test_idle_handles_clock_wrap);
    RUN_TEST(test_cleanup_rejects_invalid_cleanup);
    RUN_TEST(test_draw_decision_has_no_cleanup);
    RUN_TEST(test_cleanup_plane_inverts_flagged_centers_only);
    RUN_TEST(test_cleanup_plane_neighbor_inverts_cardinal_neighbors_of_centers);
    RUN_TEST(test_cleanup_plane_light_inverts_pixels_with_soft_debt);
    RUN_TEST(test_cleanup_plane_strong_inverts_whole_screen);
    RUN_TEST(test_changed_window_helper);
    return UNITY_END();
}
