/* Thiết kế: docs/design/display/display.md */

/* Lệnh console `display`. Chạy trên task REPL: chỉ kiểm tra cú pháp rồi gửi yêu cầu; task màn làm và log kết quả. */
#include "ink_display_internal.h"
#include "ink_display_policy.h"
#include "ink_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_EDGE_MARGIN 1 /* khi bật `display edge on` không kèm số */

/* ---- display profile ---- */

static void print_profiles(void) {
    const ink_hal_display_capabilities_t *capabilities = display_capabilities();
    const size_t                          full         = display_slot_profile(INK_HAL_DISPLAY_SLOT_FULL);
    const size_t                          partial      = display_slot_profile(INK_HAL_DISPLAY_SLOT_PARTIAL);
    for (size_t i = 0; i < capabilities->profile_count; i++) {
        printf("%-14s %5lu ms%s%s\n", capabilities->profiles[i].name, (unsigned long)capabilities->profiles[i].typical_ms,
               i == full ? "  [full]" : "", i == partial ? "  [partial]" : "");
    }
}

static int find_profile(const char *name) {
    const ink_hal_display_capabilities_t *capabilities = display_capabilities();
    for (size_t i = 0; i < capabilities->profile_count; i++) {
        if (strcmp(capabilities->profiles[i].name, name) == 0) { return (int)i; }
    }
    return -1;
}

static int on_profile_command(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[2], "list") == 0) {
        print_profiles();
        return 0;
    }
    if (argc != 5 || strcmp(argv[2], "set") != 0) {
        printf("display profile list | display profile set <full|partial> <tên>\n");
        return 1;
    }
    const int profile = find_profile(argv[4]);
    if (profile < 0) {
        printf("không có profile %s\n", argv[4]);
        return 1;
    }
    const ink_hal_display_slot_t slot = strcmp(argv[3], "full") == 0 ? INK_HAL_DISPLAY_SLOT_FULL : INK_HAL_DISPLAY_SLOT_PARTIAL;
    display_request_profile(slot, (size_t)profile);
    return 0;
}

/* ---- display edge ---- */

static int on_edge_command(int argc, char **argv) {
    uint8_t margin = 0;
    if (argc >= 3 && strcmp(argv[2], "on") == 0) {
        margin = argc >= 4 ? (uint8_t)atoi(argv[3]) : DEFAULT_EDGE_MARGIN;
    } else if (argc != 3 || strcmp(argv[2], "off") != 0) {
        printf("display edge on [số điểm 1-%d] | display edge off\n", INK_DISPLAY_POLICY_MAX_EDGE_MARGIN);
        return 1;
    }
    display_request_edge_margin(margin);
    return 0;
}

/* ---- Lệnh gốc ---- */

static int on_display_command(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "stats") == 0) {
        display_request_stats();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "temp") == 0) {
        display_request_temperature();
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "profile") == 0) { return on_profile_command(argc, argv); }
    if (argc >= 3 && strcmp(argv[1], "edge") == 0) { return on_edge_command(argc, argv); }
    printf("display stats | profile list | profile set <full|partial> <tên> | edge on [n] | edge off | temp\n");
    return 1;
}

esp_err_t ink_display_add_console_command(void) {
    if (!display_is_started()) { return ESP_ERR_INVALID_STATE; }
    return ink_platform_console_add("display", "Màn: display stats | profile | edge | temp", "stats|profile|edge|temp", on_display_command);
}
