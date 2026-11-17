/* Design: docs/design/app-framework.md */

/* Host test: runs on the PC with gcc and the Unity copy from ESP-IDF. One test_* function per case. */
#include "ink_scene.h"
#include "unity.h"

void setUp(void) {
}

void tearDown(void) {
}

/* TODO(scene): push/pop/replace/reset, FULL at depth 4, EMPTY at the root, save/restore round trip, bad snapshots. */
static void test_snapshot_fits_rtc_slot(void) {
    TEST_ASSERT_LESS_OR_EQUAL(40, sizeof(ink_scene_snapshot_t));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_snapshot_fits_rtc_slot);
    return UNITY_END();
}
