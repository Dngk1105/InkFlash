/* Design: docs/design/app-framework.md */

/* Host test: runs on the PC with gcc and the Unity copy from ESP-IDF. One test_* function per case. */
#include "ink_scene.h"
#include "unity.h"

#include <string.h>

void setUp(void) {
}

void tearDown(void) {
}

static void test_snapshot_fits_rtc_slot(void) {
    TEST_ASSERT_LESS_OR_EQUAL(40, sizeof(ink_scene_snapshot_t));
}

static void test_push_up_to_max_depth_and_full(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);

    const uint8_t state1[INK_SCENE_STATE_BYTES] = {7, 0, 0, 0, 0, 0, 0, 0};
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, state1));
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 2, nullptr));
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 3, nullptr));
    TEST_ASSERT_EQUAL(4, ink_scene_depth(&stack));

    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_FULL, ink_scene_push(&stack, 4, nullptr));
    TEST_ASSERT_EQUAL(4, ink_scene_depth(&stack));

    const ink_scene_entry_t *top = ink_scene_top(&stack);
    TEST_ASSERT_NOT_NULL(top);
    TEST_ASSERT_EQUAL(3, top->scene_id);
}

static void test_pop_stops_at_root(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, nullptr));
    TEST_ASSERT_EQUAL(2, ink_scene_depth(&stack));

    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_pop(&stack));
    TEST_ASSERT_EQUAL(1, ink_scene_depth(&stack));

    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_EMPTY, ink_scene_pop(&stack));
    TEST_ASSERT_EQUAL(1, ink_scene_depth(&stack));

    const ink_scene_entry_t *top = ink_scene_top(&stack);
    TEST_ASSERT_NOT_NULL(top);
    TEST_ASSERT_EQUAL(0, top->scene_id);
}

static void test_replace_overwrites_top_state_and_scene(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    const uint8_t state1[INK_SCENE_STATE_BYTES] = {7, 0, 0, 0, 0, 0, 0, 0};
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, state1));
    TEST_ASSERT_EQUAL(2, ink_scene_depth(&stack));

    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_replace(&stack, 2, nullptr));
    TEST_ASSERT_EQUAL(2, ink_scene_depth(&stack));

    const ink_scene_entry_t *top = ink_scene_top(&stack);
    TEST_ASSERT_NOT_NULL(top);
    TEST_ASSERT_EQUAL(2, top->scene_id);
    const uint8_t zeroes[INK_SCENE_STATE_BYTES] = {0};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(zeroes, top->state, INK_SCENE_STATE_BYTES);
}

static void test_reset_drops_above_root_and_keeps_root_state(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    const uint8_t state0[INK_SCENE_STATE_BYTES] = {5, 0, 0, 0, 0, 0, 0, 0};
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_replace(&stack, 0, state0));
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, nullptr));
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 2, nullptr));
    TEST_ASSERT_EQUAL(3, ink_scene_depth(&stack));

    ink_scene_reset(&stack);
    TEST_ASSERT_EQUAL(1, ink_scene_depth(&stack));

    const ink_scene_entry_t *top = ink_scene_top(&stack);
    TEST_ASSERT_NOT_NULL(top);
    TEST_ASSERT_EQUAL(0, top->scene_id);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(state0, top->state, INK_SCENE_STATE_BYTES);
}

static void test_save_and_restore_round_trip(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    const uint8_t state1[INK_SCENE_STATE_BYTES] = {7, 0, 0, 0, 0, 0, 0, 0};
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, state1));

    ink_scene_snapshot_t snapshot;
    ink_scene_save(&stack, &snapshot);
    TEST_ASSERT_EQUAL(2, snapshot.depth);
    TEST_ASSERT_EQUAL(0, snapshot.entries[0].scene_id);
    TEST_ASSERT_EQUAL(1, snapshot.entries[1].scene_id);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(state1, snapshot.entries[1].state, INK_SCENE_STATE_BYTES);

    ink_scene_stack_t restored;
    ink_scene_init(&restored, 99);
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_restore(&restored, &snapshot, 2));
    TEST_ASSERT_EQUAL(2, ink_scene_depth(&restored));

    const ink_scene_entry_t *entry0 = ink_scene_entry_at(&restored, 0);
    TEST_ASSERT_NOT_NULL(entry0);
    TEST_ASSERT_EQUAL(0, entry0->scene_id);

    const ink_scene_entry_t *entry1 = ink_scene_entry_at(&restored, 1);
    TEST_ASSERT_NOT_NULL(entry1);
    TEST_ASSERT_EQUAL(1, entry1->scene_id);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(state1, entry1->state, INK_SCENE_STATE_BYTES);

    ink_scene_stack_t backup;
    memcpy(&backup, &restored, sizeof(backup));
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&restored, &snapshot, 1));
    TEST_ASSERT_EQUAL(0, memcmp(&backup, &restored, sizeof(backup)));
}

static void test_null_arguments_handled_safely(void) {
    ink_scene_init(nullptr, 0);

    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_ARGUMENT, ink_scene_push(nullptr, 1, nullptr));
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_ARGUMENT, ink_scene_pop(nullptr));
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_ARGUMENT, ink_scene_replace(nullptr, 1, nullptr));

    ink_scene_reset(nullptr);

    TEST_ASSERT_NULL(ink_scene_top(nullptr));
    TEST_ASSERT_EQUAL(0, ink_scene_depth(nullptr));
    TEST_ASSERT_NULL(ink_scene_entry_at(nullptr, 0));

    ink_scene_snapshot_t snapshot;
    memset(&snapshot, 0xAA, sizeof(snapshot));
    ink_scene_snapshot_t snapshot_copy;
    memcpy(&snapshot_copy, &snapshot, sizeof(snapshot));

    ink_scene_save(nullptr, &snapshot);
    TEST_ASSERT_EQUAL(0, memcmp(&snapshot, &snapshot_copy, sizeof(snapshot)));

    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    ink_scene_save(&stack, nullptr);

    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_ARGUMENT, ink_scene_restore(nullptr, &snapshot, 2));
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_ARGUMENT, ink_scene_restore(&stack, nullptr, 2));
}

static void test_push_state_copy(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);

    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, nullptr));
    const ink_scene_entry_t *entry1 = ink_scene_entry_at(&stack, 1);
    TEST_ASSERT_NOT_NULL(entry1);
    TEST_ASSERT_EQUAL(1, entry1->scene_id);
    const uint8_t zeroes[INK_SCENE_STATE_BYTES] = {0};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(zeroes, entry1->state, INK_SCENE_STATE_BYTES);

    const uint8_t sample_state[INK_SCENE_STATE_BYTES] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 2, sample_state));
    const ink_scene_entry_t *entry2 = ink_scene_entry_at(&stack, 2);
    TEST_ASSERT_NOT_NULL(entry2);
    TEST_ASSERT_EQUAL(2, entry2->scene_id);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(sample_state, entry2->state, INK_SCENE_STATE_BYTES);
}

static void test_entry_at_beyond_depth_returns_null(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    TEST_ASSERT_EQUAL(1, ink_scene_depth(&stack));

    TEST_ASSERT_NOT_NULL(ink_scene_entry_at(&stack, 0));
    TEST_ASSERT_NULL(ink_scene_entry_at(&stack, 1));
    TEST_ASSERT_NULL(ink_scene_entry_at(&stack, 2));
    TEST_ASSERT_NULL(ink_scene_entry_at(&stack, INK_SCENE_STACK_MAX));
    TEST_ASSERT_NULL(ink_scene_entry_at(&stack, 255));
}

static void test_restore_rejects_out_of_range_depth(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);

    ink_scene_snapshot_t snapshot = {
        .depth   = 0,
        .entries = {{.scene_id = 0}},
    };
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&stack, &snapshot, 10));

    snapshot.depth = 5;
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&stack, &snapshot, 10));

    snapshot.depth = 255;
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&stack, &snapshot, 10));
}

static void test_restore_failure_leaves_stack_unchanged(void) {
    ink_scene_stack_t stack;
    ink_scene_init(&stack, 0);
    const uint8_t sample_state[INK_SCENE_STATE_BYTES] = {1, 2, 3, 4, 5, 6, 7, 8};
    TEST_ASSERT_EQUAL(INK_SCENE_OK, ink_scene_push(&stack, 1, sample_state));

    ink_scene_stack_t expected;
    memcpy(&expected, &stack, sizeof(expected));

    ink_scene_snapshot_t bad_snap = {
        .depth   = 0,
        .entries = {{.scene_id = 0}},
    };
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&stack, &bad_snap, 5));
    TEST_ASSERT_EQUAL(0, memcmp(&expected, &stack, sizeof(expected)));

    bad_snap.depth = 5;
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&stack, &bad_snap, 5));
    TEST_ASSERT_EQUAL(0, memcmp(&expected, &stack, sizeof(expected)));

    bad_snap.depth               = 1;
    bad_snap.entries[0].scene_id = 5;
    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_SNAPSHOT, ink_scene_restore(&stack, &bad_snap, 5));
    TEST_ASSERT_EQUAL(0, memcmp(&expected, &stack, sizeof(expected)));

    TEST_ASSERT_EQUAL(INK_SCENE_ERROR_ARGUMENT, ink_scene_restore(&stack, nullptr, 5));
    TEST_ASSERT_EQUAL(0, memcmp(&expected, &stack, sizeof(expected)));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_snapshot_fits_rtc_slot);
    RUN_TEST(test_push_up_to_max_depth_and_full);
    RUN_TEST(test_pop_stops_at_root);
    RUN_TEST(test_replace_overwrites_top_state_and_scene);
    RUN_TEST(test_reset_drops_above_root_and_keeps_root_state);
    RUN_TEST(test_save_and_restore_round_trip);
    RUN_TEST(test_null_arguments_handled_safely);
    RUN_TEST(test_push_state_copy);
    RUN_TEST(test_entry_at_beyond_depth_returns_null);
    RUN_TEST(test_restore_rejects_out_of_range_depth);
    RUN_TEST(test_restore_failure_leaves_stack_unchanged);
    return UNITY_END();
}
