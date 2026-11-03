/* Thiết kế: docs/design/display/display.md */

/* Host test: chạy trên PC bằng gcc, dùng Unity lấy từ ESP-IDF. Mỗi trường hợp một hàm test_*. */
#include "ink_display_policy.h"
#include "unity.h"
#include <string.h>

/* Màn nhỏ 20 × 8: mỗi hàng 3 byte, 4 bit đệm cuối hàng. */
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
        .width           = WIDTH,
        .height          = HEIGHT,
        .x_align         = 8,
        .weights         = {[INK_DISPLAY_POLICY_INTENT_TEXT] = 1, [INK_DISPLAY_POLICY_INTENT_UI] = 2, [INK_DISPLAY_POLICY_INTENT_PAGE] = 1},
        .auto_ui_percent = 10,
        .debt_soft       = 2,
        .debt_page       = 6,
        .debt_ceiling    = 8,
        .debt_ceiling_max = 15,
        .edge_ms          = 2000,
        .light_ms         = 10000,
        .strong_ms        = 60000,
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

static ink_display_policy_request_t request(ink_display_policy_intent_t intent, uint8_t flags) {
    return (ink_display_policy_request_t){.intent = intent, .flags = flags};
}

/* decide s_next rồi coi như màn hiện s_next. */
static ink_display_policy_decision_t draw_next(ink_display_policy_intent_t intent, uint8_t flags) {
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_decide(&s_state, s_shown, s_next, request(intent, flags), s_now_ms, &decision));
    memcpy(s_shown, s_next, SIZE);
    return decision;
}

/* Khởi tạo và vẽ lần đầu (dọn mạnh), để màn có khung hiện là s_shown. */
static void start(const ink_display_policy_config_t *config) {
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, config, s_pixels));
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(
        INK_DISPLAY_POLICY_OK,
        ink_display_policy_decide(&s_state, nullptr, s_shown, request(INK_DISPLAY_POLICY_INTENT_TEXT, 0), s_now_ms, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, decision.action);
}

static ink_display_policy_idle_t idle_at(uint32_t now_ms) {
    ink_display_policy_idle_t idle;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_idle(&s_state, now_ms, &idle));
    return idle;
}

/* Ô đen 2 × 2 ở (x, y) trên nền trắng của s_next. */
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

/* ---- Khởi tạo, config ---- */

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
    changed           = config;
    changed.debt_soft = 3;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_set_config(&s_state, &changed));
}

static void test_decide_rejects_bad_arguments(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_decide(&s_state, nullptr, s_next, request(INK_DISPLAY_POLICY_INTENT_TEXT, 0), 0, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_decide(&s_state, s_shown, s_next, request(INK_DISPLAY_POLICY_INTENT_COUNT, 0), 0, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_decide(&s_state, s_shown, s_next, request(INK_DISPLAY_POLICY_INTENT_TEXT, 0), 0, nullptr));
}

/* ---- Chọn cách vẽ ---- */

static void test_first_decide_is_clean_strong_with_inverted_plane(void) {
    const ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, &config, s_pixels));
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_decide(&s_state, nullptr, s_next, request(INK_DISPLAY_POLICY_INTENT_UI, 0), 0, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(WIDTH, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(HEIGHT, decision.window.height);
}

static void test_identical_frame_draws_nothing(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, decision.action);
    TEST_ASSERT_EQUAL_UINT16(0, decision.window.width);
}

static void test_draw_uses_changed_window_and_shown_plane(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 10, 3);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_INTENT_TEXT, decision.intent);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SHOWN, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(8, decision.window.x); /* căn theo x_align 8 */
    TEST_ASSERT_EQUAL_UINT16(8, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(3, decision.window.y);
    TEST_ASSERT_EQUAL_UINT16(1, decision.window.height);
}

static void test_window_is_clipped_to_screen_width(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 19, 0);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL_UINT16(16, decision.window.x);
    TEST_ASSERT_EQUAL_UINT16(4, decision.window.width);
}

static void test_auto_picks_ui_for_small_window_and_text_for_large(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 2, 2); /* window 8 × 1 = 5 % màn */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_INTENT_UI, draw_next(INK_DISPLAY_POLICY_INTENT_AUTO, 0).intent);
    memset(s_next, 0x00, SIZE); /* cả màn đổi */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_INTENT_TEXT, draw_next(INK_DISPLAY_POLICY_INTENT_AUTO, 0).intent);
}

static void test_after_wake_draws_whole_screen_with_intent(void) {
    const ink_display_policy_config_t config = default_config();
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_init(&s_state, &config, s_pixels));
    ink_display_policy_assume_shown(&s_state);
    set_black(s_next, 1, 1);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_POLICY_INTENT_PAGE, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_DRAW, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_INTENT_PAGE, decision.intent);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_SHOWN, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT16(WIDTH, decision.window.width);
    TEST_ASSERT_EQUAL_UINT16(HEIGHT, decision.window.height);
    set_black(s_next, 12, 1);
    TEST_ASSERT_EQUAL_UINT16(8, draw_next(INK_DISPLAY_POLICY_INTENT_PAGE, 0).window.width);
}

static void test_invalidate_forces_clean_strong(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_invalidate(&s_state);
    ink_display_policy_decision_t decision;
    TEST_ASSERT_EQUAL(
        INK_DISPLAY_POLICY_OK,
        ink_display_policy_decide(&s_state, nullptr, s_next, request(INK_DISPLAY_POLICY_INTENT_TEXT, 0), s_now_ms, &decision));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, decision.action);
}

/* ---- Nợ ---- */

static void test_debt_adds_weight_on_changed_pixels_only(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(2, ink_display_policy_max_debt(&s_state));
    set_white(s_next, 4, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0);
    TEST_ASSERT_EQUAL_UINT8(3, ink_display_policy_max_debt(&s_state));
    TEST_ASSERT_EQUAL_UINT8(3 << 2, s_pixels[4 * WIDTH + 4] & 0x3C);
    TEST_ASSERT_EQUAL_UINT8(0, s_pixels[4 * WIDTH + 5] & 0x3C); /* trong window nhưng không đổi màu */
}

static void test_exceeding_ceiling_draws_clean_strong_and_clears_debt(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    /* nghỉ lâu giữa các lần lật: trần giữ 8; UI cộng 2 mỗi lần */
    for (int i = 0; i < 4; i++) {
        s_now_ms += 5000;
        if (i % 2 == 0) {
            set_black(s_next, 4, 4);
        } else {
            set_white(s_next, 4, 4);
        }
        TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_DRAW, draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0).action);
    }
    TEST_ASSERT_EQUAL_UINT8(8, ink_display_policy_max_debt(&s_state));
    s_now_ms += 5000;
    set_black(s_next, 4, 4);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
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
        TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_DRAW, draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0).action);
    }
    TEST_ASSERT_EQUAL_UINT8(14, ink_display_policy_max_debt(&s_state));
    TEST_ASSERT_EQUAL_UINT8(15, ink_display_policy_ceiling(&s_state));
    s_now_ms += 5000; /* nghỉ quá edge_ms: trần về 8, nợ 14 + 2 vượt */
    set_white(s_next, 4, 4);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0).action);
    TEST_ASSERT_EQUAL_UINT8(8, ink_display_policy_ceiling(&s_state));
}

static void test_page_with_high_debt_draws_clean_fast(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    for (int i = 0; i < 3; i++) {
        s_now_ms += 100;
        if (i % 2 == 0) {
            set_black(s_next, 4, 4);
        } else {
            set_white(s_next, 4, 4);
        }
        draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    }
    TEST_ASSERT_EQUAL_UINT8(6, ink_display_policy_max_debt(&s_state));
    set_black(s_next, 15, 6);
    const ink_display_policy_decision_t decision = draw_next(INK_DISPLAY_POLICY_INTENT_PAGE, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_FAST, decision.action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OLD_INVERTED, decision.old_plane);
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));
}

static void test_before_sleep_cleans_only_when_dirty(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 2, 2); /* màn sạch: vẽ bình thường */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_DRAW,
                      draw_next(INK_DISPLAY_POLICY_INTENT_PAGE, INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP).action);
    set_black(s_next, 12, 2); /* còn nợ của lần trước */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG,
                      draw_next(INK_DISPLAY_POLICY_INTENT_PAGE, INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP).action);
}

/* ---- Cờ viền ---- */

static void test_marks_dark_edge_around_box_that_turns_white(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    /* xét theo hàng: (4, 2) kề (4, 3) vừa thôi đen nên có cờ viền tối; (3, 3) có (4, 2) ở góc nên không (chống cụm) */
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[2 * WIDTH + 4] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(0, s_pixels[3 * WIDTH + 3] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[3 * WIDTH + 6] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[4 * WIDTH + 3] & 0x03);
    TEST_ASSERT_EQUAL_UINT8(1, s_pixels[5 * WIDTH + 5] & 0x03);
    TEST_ASSERT_EQUAL_UINT32(4, ink_display_policy_flagged_count(&s_state));
}

static void test_marks_light_edge_next_to_pixel_that_turns_black(void) {
    const ink_display_policy_config_t config = default_config();
    set_black(s_shown, 5, 5);
    start(&config);
    memcpy(s_next, s_shown, SIZE);
    set_black(s_next, 6, 5);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(2, s_pixels[5 * WIDTH + 5] & 0x03);
}

static void test_changed_pixel_loses_its_flag(void) {
    const ink_display_policy_config_t config = default_config();
    set_black(s_shown, 5, 5);
    start(&config);
    memcpy(s_next, s_shown, SIZE);
    set_black(s_next, 6, 5);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    const uint32_t flagged = ink_display_policy_flagged_count(&s_state);
    set_white(s_next, 5, 5);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL_UINT8(0, s_pixels[5 * WIDTH + 5] & 0x03);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(flagged + 4, ink_display_policy_flagged_count(&s_state));
}

/* ---- Thang dọn ---- */

static void test_idle_ladder_runs_edge_then_light_then_strong(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    const uint32_t drawn_ms = s_now_ms;

    ink_display_policy_idle_t idle = idle_at(drawn_ms + 500);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, idle.action);
    TEST_ASSERT_EQUAL_UINT32(1500, idle.wait_ms);

    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER, idle_at(drawn_ms + 2000).action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_NEIGHBOR, idle_at(drawn_ms + 2100).action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_NEIGHBOR));
    TEST_ASSERT_EQUAL_UINT32(0, ink_display_policy_flagged_count(&s_state));

    idle = idle_at(drawn_ms + 2200);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, idle.action);
    TEST_ASSERT_EQUAL_UINT32(7800, idle.wait_ms);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT, idle_at(drawn_ms + 10000).action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT));
    TEST_ASSERT_EQUAL_UINT8(0, ink_display_policy_max_debt(&s_state));

    /* hết nợ và cờ: không còn bậc nào */
    idle = idle_at(drawn_ms + 70000);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, idle.action);
    TEST_ASSERT_EQUAL_UINT32(INK_DISPLAY_POLICY_NO_WAIT, idle.wait_ms);
}

static void test_idle_strong_runs_when_debt_below_soft_remains(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0); /* nợ 1 < debt_soft */
    const uint32_t drawn_ms = s_now_ms;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, idle_at(drawn_ms + 30000).action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, idle_at(drawn_ms + 60000).action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG));
    TEST_ASSERT_EQUAL_UINT32(INK_DISPLAY_POLICY_NO_WAIT, idle_at(drawn_ms + 60001).wait_ms);
}

static void test_new_frame_restarts_ladder(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER));
    s_now_ms += 2100;
    set_black(s_next, 15, 0);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    /* lượt dọn tâm phải chạy lại: thang bắt đầu lại từ khung mới */
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, idle_at(s_now_ms + 1000).action);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER, idle_at(s_now_ms + 2000).action);
}

static void test_defer_flag_blocks_ladder_until_next_frame(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_NONE, idle_at(s_now_ms + 100000).action);
    TEST_ASSERT_EQUAL_UINT32(INK_DISPLAY_POLICY_NO_WAIT, idle_at(s_now_ms + 100000).wait_ms);
    set_white(s_next, 4, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER, idle_at(s_now_ms + 2000).action);
}

static void test_zero_stage_time_skips_stage(void) {
    ink_display_policy_config_t config = default_config();
    config.edge_ms                     = 0;
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT, idle_at(s_now_ms + 10000).action);
}

static void test_idle_handles_clock_wrap(void) {
    const ink_display_policy_config_t config = default_config();
    s_now_ms                                 = UINT32_MAX - 500;
    start(&config);
    set_black(s_next, 4, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT, idle_at(s_now_ms + 10000).action);
}

/* ---- Mặt phẳng cũ của lượt dọn ---- */

static void test_cleanup_plane_inverts_flagged_centers_only(void) {
    const ink_display_policy_config_t config = default_config();
    black_box(4, 3);
    memcpy(s_shown, s_next, SIZE);
    start(&config);
    memset(s_next, 0xFF, SIZE);
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    ink_display_policy_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER, s_old, &window));
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
    draw_next(INK_DISPLAY_POLICY_INTENT_UI, 0);
    ink_display_policy_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_NEIGHBOR, s_old, &window));
    /* (3, 4) là tâm: không đảo; (3, 3) ngay trên, trắng, không cờ: đảo */
    TEST_ASSERT_TRUE(is_white(s_old, 3, 4));
    TEST_ASSERT_FALSE(is_white(s_old, 3, 3));
}

static void test_cleanup_plane_light_inverts_pixels_with_soft_debt(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    set_black(s_next, 4, 4);
    set_black(s_next, 6, 4);
    draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0);
    s_now_ms += 100;
    set_white(s_next, 4, 4); /* (4, 4) nợ 2, (6, 4) nợ 1 */
    draw_next(INK_DISPLAY_POLICY_INTENT_TEXT, 0);
    ink_display_policy_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT, s_old, &window));
    TEST_ASSERT_FALSE(is_white(s_old, 4, 4));
    TEST_ASSERT_FALSE(is_white(s_old, 6, 4)); /* đen, không đảo */
    TEST_ASSERT_EQUAL_UINT16(0, window.x);
    TEST_ASSERT_EQUAL_UINT16(8, window.width);
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT));
    TEST_ASSERT_EQUAL_UINT8(1, ink_display_policy_max_debt(&s_state));
}

static void test_cleanup_plane_strong_inverts_whole_screen(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_OK,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG, s_old, &window));
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) { TEST_ASSERT_NOT_EQUAL(is_white(s_shown, x, y), is_white(s_old, x, y)); }
    }
    TEST_ASSERT_EQUAL_UINT16(WIDTH, window.width);
    TEST_ASSERT_EQUAL_UINT16(HEIGHT, window.height);
}

static void test_cleanup_rejects_non_cleanup_action(void) {
    const ink_display_policy_config_t config = default_config();
    start(&config);
    ink_display_policy_rect_t window;
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT,
                      ink_display_policy_cleanup_plane(&s_state, s_shown, INK_DISPLAY_POLICY_ACTION_DRAW, s_old, &window));
    TEST_ASSERT_EQUAL(INK_DISPLAY_POLICY_ERROR_ARGUMENT, ink_display_policy_cleanup_done(&s_state, INK_DISPLAY_POLICY_ACTION_CLEAN_FAST));
}

/* ---- Vùng đổi ---- */

static void test_changed_window_helper(void) {
    const ink_display_policy_config_t config = default_config();
    ink_display_policy_rect_t         window;
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
    RUN_TEST(test_first_decide_is_clean_strong_with_inverted_plane);
    RUN_TEST(test_identical_frame_draws_nothing);
    RUN_TEST(test_draw_uses_changed_window_and_shown_plane);
    RUN_TEST(test_window_is_clipped_to_screen_width);
    RUN_TEST(test_auto_picks_ui_for_small_window_and_text_for_large);
    RUN_TEST(test_after_wake_draws_whole_screen_with_intent);
    RUN_TEST(test_invalidate_forces_clean_strong);
    RUN_TEST(test_debt_adds_weight_on_changed_pixels_only);
    RUN_TEST(test_exceeding_ceiling_draws_clean_strong_and_clears_debt);
    RUN_TEST(test_ceiling_grows_while_active_and_resets_after_rest);
    RUN_TEST(test_page_with_high_debt_draws_clean_fast);
    RUN_TEST(test_before_sleep_cleans_only_when_dirty);
    RUN_TEST(test_marks_dark_edge_around_box_that_turns_white);
    RUN_TEST(test_marks_light_edge_next_to_pixel_that_turns_black);
    RUN_TEST(test_changed_pixel_loses_its_flag);
    RUN_TEST(test_idle_ladder_runs_edge_then_light_then_strong);
    RUN_TEST(test_idle_strong_runs_when_debt_below_soft_remains);
    RUN_TEST(test_new_frame_restarts_ladder);
    RUN_TEST(test_defer_flag_blocks_ladder_until_next_frame);
    RUN_TEST(test_zero_stage_time_skips_stage);
    RUN_TEST(test_idle_handles_clock_wrap);
    RUN_TEST(test_cleanup_plane_inverts_flagged_centers_only);
    RUN_TEST(test_cleanup_plane_neighbor_inverts_cardinal_neighbors_of_centers);
    RUN_TEST(test_cleanup_plane_light_inverts_pixels_with_soft_debt);
    RUN_TEST(test_cleanup_plane_strong_inverts_whole_screen);
    RUN_TEST(test_cleanup_rejects_non_cleanup_action);
    RUN_TEST(test_changed_window_helper);
    return UNITY_END();
}
