/* Design: docs/design/wake-session.md */

/* Host test: runs on the PC with gcc and the Unity copy from ESP-IDF. One test_* function per case. */
#include "ink_session.h"
#include "unity.h"

#include <string.h>

#define IDLE_MS 60000u

static ink_session_state_t s_state;

void setUp(void) {
    TEST_ASSERT_EQUAL(INK_SESSION_OK, ink_session_init(&s_state, IDLE_MS));
}

void tearDown(void) {
}

static ink_session_output_t send(ink_session_event_type_t type, uint32_t time_ms, uint32_t value) {
    const ink_session_event_t event = {.type = type, .time_ms = time_ms, .value = value};
    ink_session_output_t      out;
    TEST_ASSERT_EQUAL(INK_SESSION_OK, ink_session_handle(&s_state, &event, &out));
    return out;
}

/* Boot (cold) and restore at `time_ms`: the session is ACTIVE afterwards. */
static void go_active(uint32_t time_ms) {
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_COLD_BOOT, send(INK_SESSION_EVENT_BOOT, time_ms, 0).action);
    const ink_session_output_t out = send(INK_SESSION_EVENT_RESTORED, time_ms, 0);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_ARM_TIMER, out.action);
    TEST_ASSERT_EQUAL_UINT32(IDLE_MS, out.delay_ms);
    TEST_ASSERT_EQUAL(INK_SESSION_PHASE_ACTIVE, ink_session_phase(&s_state));
}

static void test_init_rejects_bad_arguments(void) {
    ink_session_state_t state;
    TEST_ASSERT_EQUAL(INK_SESSION_ERROR_ARGUMENT, ink_session_init(&state, 0));
    TEST_ASSERT_EQUAL(INK_SESSION_ERROR_ARGUMENT, ink_session_init(nullptr, IDLE_MS));
}

static void test_handle_rejects_bad_arguments(void) {
    ink_session_output_t      out;
    const ink_session_event_t bad = {.type = INK_SESSION_EVENT_COUNT};
    TEST_ASSERT_EQUAL(INK_SESSION_ERROR_ARGUMENT, ink_session_handle(&s_state, nullptr, &out));
    TEST_ASSERT_EQUAL(INK_SESSION_ERROR_ARGUMENT, ink_session_handle(&s_state, &bad, &out));
}

static void test_deep_sleep_wake_restores(void) {
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_RESTORE, send(INK_SESSION_EVENT_BOOT, 0, 1).action);
    TEST_ASSERT_EQUAL(INK_SESSION_PHASE_RESTORE, ink_session_phase(&s_state));
}

static void test_events_before_restored_are_ignored(void) {
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_ACTIVITY, 0, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_SLEEP, 0, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_COLD_BOOT, send(INK_SESSION_EVENT_BOOT, 0, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_BOOT, 0, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_TIMER, 0, 0).action);
}

static void test_activity_pushes_deadline(void) {
    go_active(1000);
    const ink_session_output_t out = send(INK_SESSION_EVENT_ACTIVITY, 21000, 0);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_ARM_TIMER, out.action);
    TEST_ASSERT_EQUAL_UINT32(IDLE_MS, out.delay_ms);
    /* The timer armed at RESTORED fires at 61000: 20 s are still left. */
    const ink_session_output_t early = send(INK_SESSION_EVENT_TIMER, 61000, 0);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_ARM_TIMER, early.action);
    TEST_ASSERT_EQUAL_UINT32(20000, early.delay_ms);
}

static void test_timer_at_deadline_saves_then_sleeps(void) {
    go_active(0);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_SAVE, send(INK_SESSION_EVENT_TIMER, IDLE_MS, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_PHASE_SAVE, ink_session_phase(&s_state));
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_DEEP_SLEEP, send(INK_SESSION_EVENT_POWER_OFF, IDLE_MS + 500, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_PHASE_SLEEP, ink_session_phase(&s_state));
}

static void test_events_while_saving_are_ignored(void) {
    go_active(0);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_SAVE, send(INK_SESSION_EVENT_SLEEP, 100, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_ACTIVITY, 200, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_TIMER, 300, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_KEEP_AWAKE, 300, 1000).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_SLEEP, 300, 0).action);
}

static void test_power_off_while_active_is_ignored(void) {
    go_active(0);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_NONE, send(INK_SESSION_EVENT_POWER_OFF, 100, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_PHASE_ACTIVE, ink_session_phase(&s_state));
}

static void test_keep_awake_is_capped(void) {
    go_active(0);
    /* 50 s after the last activity, ask for 100 s: capped to 30 s, so the deadline moves from 60 s to 80 s. */
    const ink_session_output_t out = send(INK_SESSION_EVENT_KEEP_AWAKE, 50000, 100000);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_ARM_TIMER, out.action);
    TEST_ASSERT_EQUAL_UINT32(INK_SESSION_KEEP_AWAKE_MAX_MS, out.delay_ms);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_ARM_TIMER, send(INK_SESSION_EVENT_TIMER, 60000, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_SAVE, send(INK_SESSION_EVENT_TIMER, 80000, 0).action);
}

static void test_short_keep_awake_does_not_shorten(void) {
    go_active(0);
    const ink_session_output_t out = send(INK_SESSION_EVENT_KEEP_AWAKE, 1000, 1000);
    TEST_ASSERT_EQUAL_UINT32(IDLE_MS - 1000, out.delay_ms);
}

static void test_deadline_across_clock_wrap(void) {
    go_active(UINT32_MAX - 1000);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_ARM_TIMER, send(INK_SESSION_EVENT_TIMER, 5000, 0).action);
    TEST_ASSERT_EQUAL(INK_SESSION_ACTION_SAVE, send(INK_SESSION_EVENT_TIMER, IDLE_MS - 1001, 0).action);
}

/* ---- RTC slot header ---- */

static uint32_t sum_crc(const uint8_t *data, size_t length) {
    uint32_t sum = 0x1234;
    for (size_t i = 0; i < length; i++) { sum = sum * 31 + data[i]; }
    return sum;
}

#define SLOT_SIZE 32

static void seal_sample(uint8_t *slot) {
    memset(slot, 0, SLOT_SIZE);
    uint8_t *payload = ink_session_slot_payload(slot, SLOT_SIZE);
    TEST_ASSERT_NOT_NULL(payload);
    memcpy(payload, "abcd", 4);
    TEST_ASSERT_EQUAL(INK_SESSION_OK, ink_session_slot_seal(slot, SLOT_SIZE, 3, 4, sum_crc));
}

static void test_slot_round_trip(void) {
    uint8_t slot[SLOT_SIZE];
    seal_sample(slot);
    size_t length = 99;
    TEST_ASSERT_TRUE(ink_session_slot_check(slot, SLOT_SIZE, 3, sum_crc, &length));
    TEST_ASSERT_EQUAL_size_t(4, length);
}

static void test_slot_rejects_wrong_version(void) {
    uint8_t slot[SLOT_SIZE];
    seal_sample(slot);
    size_t length = 99;
    TEST_ASSERT_FALSE(ink_session_slot_check(slot, SLOT_SIZE, 4, sum_crc, &length));
    TEST_ASSERT_EQUAL_size_t(0, length);
}

static void test_slot_rejects_corrupt_payload(void) {
    uint8_t slot[SLOT_SIZE];
    seal_sample(slot);
    ink_session_slot_payload(slot, SLOT_SIZE)[1] ^= 0x01;
    TEST_ASSERT_FALSE(ink_session_slot_check(slot, SLOT_SIZE, 3, sum_crc, nullptr));
}

static void test_slot_rejects_bad_magic_and_length(void) {
    uint8_t slot[SLOT_SIZE];
    seal_sample(slot);
    slot[0] ^= 0xFF;
    TEST_ASSERT_FALSE(ink_session_slot_check(slot, SLOT_SIZE, 3, sum_crc, nullptr));
    seal_sample(slot);
    ink_session_slot_header_t header;
    memcpy(&header, slot, sizeof(header));
    header.length = SLOT_SIZE; /* longer than the payload area */
    memcpy(slot, &header, sizeof(header));
    TEST_ASSERT_FALSE(ink_session_slot_check(slot, SLOT_SIZE, 3, sum_crc, nullptr));
}

static void test_slot_seal_rejects_oversize(void) {
    uint8_t slot[SLOT_SIZE];
    TEST_ASSERT_EQUAL(INK_SESSION_ERROR_ARGUMENT, ink_session_slot_seal(slot, SLOT_SIZE, 1, SLOT_SIZE, sum_crc));
    TEST_ASSERT_EQUAL(INK_SESSION_ERROR_ARGUMENT, ink_session_slot_seal(slot, SLOT_SIZE, 1, 4, nullptr));
    TEST_ASSERT_NULL(ink_session_slot_payload(slot, sizeof(ink_session_slot_header_t)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_bad_arguments);
    RUN_TEST(test_handle_rejects_bad_arguments);
    RUN_TEST(test_deep_sleep_wake_restores);
    RUN_TEST(test_events_before_restored_are_ignored);
    RUN_TEST(test_activity_pushes_deadline);
    RUN_TEST(test_timer_at_deadline_saves_then_sleeps);
    RUN_TEST(test_events_while_saving_are_ignored);
    RUN_TEST(test_power_off_while_active_is_ignored);
    RUN_TEST(test_keep_awake_is_capped);
    RUN_TEST(test_short_keep_awake_does_not_shorten);
    RUN_TEST(test_deadline_across_clock_wrap);
    RUN_TEST(test_slot_round_trip);
    RUN_TEST(test_slot_rejects_wrong_version);
    RUN_TEST(test_slot_rejects_corrupt_payload);
    RUN_TEST(test_slot_rejects_bad_magic_and_length);
    RUN_TEST(test_slot_seal_rejects_oversize);
    return UNITY_END();
}
