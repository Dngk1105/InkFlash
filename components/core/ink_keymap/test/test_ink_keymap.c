/* Thiết kế: docs/design/input.md */

/* Host test: chạy trên PC bằng gcc, dùng Unity lấy từ ESP-IDF. Mỗi trường hợp một hàm test_*. */
#include "ink_keymap.h"
#include "unity.h"

void setUp(void) {
}

void tearDown(void) {
}

/* ---- Bảng mẫu ---- */

enum {
    INTENT_GRADE_GOOD = INK_KEYMAP_INTENT_APP_BASE,
    INTENT_GRADE_AGAIN
};

#define G(mod, btn, typ) {.modifier = (mod), .source = INK_GESTURE_SOURCE_BUTTON, .button = (btn), .type = (typ)}

static const ink_gesture_t B_PRESS  = G(INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_PRESS);
static const ink_gesture_t A_PRESS  = G(INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_A, INK_GESTURE_TYPE_PRESS);
static const ink_gesture_t B_HOLD   = G(INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_HOLD);
static const ink_gesture_t A_HOLD   = G(INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_A, INK_GESTURE_TYPE_HOLD);
static const ink_gesture_t B_DOUBLE = G(INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_DOUBLE_PRESS);
static const ink_gesture_t A_THEN_B = G(INK_GESTURE_MODIFIER_A, INK_GESTURE_BUTTON_B, INK_GESTURE_TYPE_PRESS);

/* App Flashcard mẫu */
static ink_keymap_table_t make_app(void) {
    return (ink_keymap_table_t){
        .count   = 4,
        .entries = {{B_PRESS, INK_KEYMAP_INTENT_NEXT},
                    {A_PRESS, INK_KEYMAP_INTENT_PREV},
                    {B_HOLD, INK_KEYMAP_INTENT_SELECT},
                    {A_HOLD, INK_KEYMAP_INTENT_BACK}}
    };
}

static ink_keymap_intent_t lookup_ok(const ink_keymap_table_t *scene, const ink_keymap_table_t *app, ink_gesture_t gesture) {
    ink_keymap_intent_t intent;
    TEST_ASSERT_TRUE(ink_keymap_lookup(scene, app, &gesture, &intent));
    return intent;
}

/* ---- Mã cố định ---- */

static void test_intent_codes_are_fixed(void) {
    TEST_ASSERT_EQUAL(0, INK_KEYMAP_INTENT_NEXT);
    TEST_ASSERT_EQUAL(1, INK_KEYMAP_INTENT_PREV);
    TEST_ASSERT_EQUAL(2, INK_KEYMAP_INTENT_SELECT);
    TEST_ASSERT_EQUAL(3, INK_KEYMAP_INTENT_BACK);
    TEST_ASSERT_EQUAL(16, INK_KEYMAP_INTENT_APP_BASE);
}

/* ---- Tra ---- */

static void test_lookup_app_table(void) {
    ink_keymap_table_t app = make_app();
    TEST_ASSERT_EQUAL(INK_KEYMAP_INTENT_NEXT, lookup_ok(nullptr, &app, B_PRESS));
    TEST_ASSERT_EQUAL(INK_KEYMAP_INTENT_BACK, lookup_ok(nullptr, &app, A_HOLD));
}

static void test_lookup_unassigned_gesture_is_dropped(void) {
    ink_keymap_table_t  app = make_app();
    ink_keymap_intent_t intent;
    TEST_ASSERT_FALSE(ink_keymap_lookup(nullptr, &app, &B_DOUBLE, &intent));
}

static void test_lookup_scene_overrides_app(void) {
    ink_keymap_table_t app   = make_app();
    ink_keymap_table_t scene = {.count = 1, .entries = {{B_PRESS, (ink_keymap_intent_t)INTENT_GRADE_GOOD}}};
    TEST_ASSERT_EQUAL(INTENT_GRADE_GOOD, lookup_ok(&scene, &app, B_PRESS));
    TEST_ASSERT_EQUAL(INK_KEYMAP_INTENT_PREV, lookup_ok(&scene, &app, A_PRESS)); /* không có trong scene: rơi về app */
}

static void test_lookup_modifier_is_part_of_gesture(void) {
    ink_keymap_table_t  app = make_app();
    ink_keymap_intent_t intent;
    TEST_ASSERT_FALSE(ink_keymap_lookup(nullptr, &app, &A_THEN_B, &intent)); /* B nhấn có modifier A khác B nhấn */
}

static void test_lookup_rejects_null(void) {
    ink_keymap_table_t  app = make_app();
    ink_keymap_intent_t intent;
    TEST_ASSERT_FALSE(ink_keymap_lookup(nullptr, &app, nullptr, &intent));
    TEST_ASSERT_FALSE(ink_keymap_lookup(nullptr, &app, &B_PRESS, nullptr));
    TEST_ASSERT_FALSE(ink_keymap_lookup(nullptr, nullptr, &B_PRESS, &intent));
}

/* ---- Kiểm tra ---- */

static void test_validate_accepts_sample(void) {
    ink_keymap_table_t app = make_app();
    TEST_ASSERT_EQUAL(INK_KEYMAP_OK, ink_keymap_validate(nullptr, &app));
}

static void test_validate_rejects_null_app(void) {
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_ARGUMENT, ink_keymap_validate(nullptr, nullptr));
}

static void test_validate_rejects_duplicate_gesture(void) {
    ink_keymap_table_t app   = make_app();
    app.entries[app.count++] = (ink_keymap_entry_t){B_PRESS, INK_KEYMAP_INTENT_SELECT};
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_DUPLICATE, ink_keymap_validate(nullptr, &app));
}

static void test_validate_allows_many_gestures_for_one_intent(void) {
    ink_keymap_table_t app   = make_app();
    app.entries[app.count++] = (ink_keymap_entry_t){B_DOUBLE, INK_KEYMAP_INTENT_NEXT};
    TEST_ASSERT_EQUAL(INK_KEYMAP_OK, ink_keymap_validate(nullptr, &app));
}

static void test_validate_rejects_system_gestures(void) {
    const ink_gesture_type_t types[] = {INK_GESTURE_TYPE_HOLD_AB_SHORT, INK_GESTURE_TYPE_HOLD_AB_LONG};
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        ink_keymap_table_t  app      = make_app();
        const ink_gesture_t reserved = G(INK_GESTURE_MODIFIER_NONE, INK_GESTURE_BUTTON_AB, types[i]);
        app.entries[app.count++]     = (ink_keymap_entry_t){reserved, INK_KEYMAP_INTENT_SELECT};
        TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_RESERVED, ink_keymap_validate(nullptr, &app));
    }
}

static void test_validate_rejects_count_over_max(void) {
    ink_keymap_table_t app = make_app();
    app.count              = INK_KEYMAP_ENTRY_MAX + 1;
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_COUNT, ink_keymap_validate(nullptr, &app));
}

static void test_validate_requires_back(void) {
    ink_keymap_table_t app = make_app();
    app.entries[3].intent  = INK_KEYMAP_INTENT_SELECT; /* A giữ không còn là BACK */
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_NO_BACK, ink_keymap_validate(nullptr, &app));
}

static void test_validate_rejects_scene_that_shadows_back(void) {
    ink_keymap_table_t app   = make_app();
    ink_keymap_table_t scene = {.count = 1, .entries = {{A_HOLD, (ink_keymap_intent_t)INTENT_GRADE_AGAIN}}};
    TEST_ASSERT_EQUAL(INK_KEYMAP_OK, ink_keymap_validate(nullptr, &app));
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_NO_BACK, ink_keymap_validate(&scene, &app));
}

static void test_validate_accepts_scene_that_moves_back(void) {
    ink_keymap_table_t app   = make_app();
    ink_keymap_table_t scene = {
        .count = 2, .entries = {{A_HOLD, (ink_keymap_intent_t)INTENT_GRADE_AGAIN}, {B_DOUBLE, INK_KEYMAP_INTENT_BACK}}
    };
    TEST_ASSERT_EQUAL(INK_KEYMAP_OK, ink_keymap_validate(&scene, &app));
}

static void test_validate_allows_scene_to_reuse_app_gesture(void) {
    ink_keymap_table_t app   = make_app();
    ink_keymap_table_t scene = {.count = 1, .entries = {{B_PRESS, (ink_keymap_intent_t)INTENT_GRADE_GOOD}}};
    TEST_ASSERT_EQUAL(INK_KEYMAP_OK, ink_keymap_validate(&scene, &app));
}

/* ---- Khôi phục mặc định ---- */

static void test_reset_restores_defaults(void) {
    const ink_keymap_table_t defaults = make_app();
    ink_keymap_table_t       table    = make_app();
    table.entries[0].intent           = INK_KEYMAP_INTENT_BACK;
    table.count                       = 2;
    TEST_ASSERT_EQUAL(INK_KEYMAP_OK, ink_keymap_reset(&table, &defaults));
    TEST_ASSERT_EQUAL(defaults.count, table.count);
    TEST_ASSERT_EQUAL(INK_KEYMAP_INTENT_NEXT, lookup_ok(nullptr, &table, B_PRESS));
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_ARGUMENT, ink_keymap_reset(nullptr, &defaults));
    TEST_ASSERT_EQUAL(INK_KEYMAP_ERROR_ARGUMENT, ink_keymap_reset(&table, nullptr));
}

/* ---- Mask kiểu phải chờ ---- */

static void test_wait_mask_defaults_to_none(void) {
    ink_keymap_table_t app = make_app();
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_NONE, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_A));
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_NONE, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_B));
}

static void test_wait_mask_double_only_on_that_button(void) {
    ink_keymap_table_t app   = make_app();
    app.entries[app.count++] = (ink_keymap_entry_t){B_DOUBLE, INK_KEYMAP_INTENT_NEXT};
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_DOUBLE, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_B));
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_NONE, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_A));
}

static void test_wait_mask_modifier_on_held_button(void) {
    ink_keymap_table_t app   = make_app();
    app.entries[app.count++] = (ink_keymap_entry_t){A_THEN_B, (ink_keymap_intent_t)INTENT_GRADE_GOOD};
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_MODIFIER, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_A));
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_NONE, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_B));
}

static void test_wait_mask_merges_scene_and_app(void) {
    ink_keymap_table_t app   = make_app();
    ink_keymap_table_t scene = {.count = 1, .entries = {{B_DOUBLE, INK_KEYMAP_INTENT_NEXT}}};
    app.entries[app.count++] = (ink_keymap_entry_t){A_THEN_B, (ink_keymap_intent_t)INTENT_GRADE_GOOD};
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_DOUBLE, ink_keymap_wait_mask(&scene, &app, INK_GESTURE_BUTTON_B));
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_MODIFIER, ink_keymap_wait_mask(&scene, &app, INK_GESTURE_BUTTON_A));
}

static void test_wait_mask_ignores_non_button_trigger(void) {
    ink_keymap_table_t app = make_app();
    TEST_ASSERT_EQUAL(INK_GESTURE_WAIT_NONE, ink_keymap_wait_mask(nullptr, &app, INK_GESTURE_BUTTON_AB));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_intent_codes_are_fixed);
    RUN_TEST(test_lookup_app_table);
    RUN_TEST(test_lookup_unassigned_gesture_is_dropped);
    RUN_TEST(test_lookup_scene_overrides_app);
    RUN_TEST(test_lookup_modifier_is_part_of_gesture);
    RUN_TEST(test_lookup_rejects_null);
    RUN_TEST(test_validate_accepts_sample);
    RUN_TEST(test_validate_rejects_null_app);
    RUN_TEST(test_validate_rejects_duplicate_gesture);
    RUN_TEST(test_validate_allows_many_gestures_for_one_intent);
    RUN_TEST(test_validate_rejects_system_gestures);
    RUN_TEST(test_validate_rejects_count_over_max);
    RUN_TEST(test_validate_requires_back);
    RUN_TEST(test_validate_rejects_scene_that_shadows_back);
    RUN_TEST(test_validate_accepts_scene_that_moves_back);
    RUN_TEST(test_validate_allows_scene_to_reuse_app_gesture);
    RUN_TEST(test_reset_restores_defaults);
    RUN_TEST(test_wait_mask_defaults_to_none);
    RUN_TEST(test_wait_mask_double_only_on_that_button);
    RUN_TEST(test_wait_mask_modifier_on_held_button);
    RUN_TEST(test_wait_mask_merges_scene_and_app);
    RUN_TEST(test_wait_mask_ignores_non_button_trigger);
    return UNITY_END();
}
