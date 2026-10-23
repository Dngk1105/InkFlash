#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "esp_check.h"
#include "esp_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_app.h"
#include "ink_bus.h"
#include "ink_device.h"
#include "ink_health.h"
#include "ink_heap.h"
#include "ink_log.h"

static int cmd_status(int argc, char **argv)
{
    printf("Component:\n");
    for (int c = 0; c < INK_COMP_COUNT; c++) {
        ink_health_t h = ink_health_get(c);
        if (h != INK_HEALTH_UNKNOWN) {
            printf("  %-14s %-9s loi=%lu\n", ink_comp_name(c), ink_health_str(h),
                   (unsigned long)ink_health_fail_count(c));
        }
    }
    printf("Bus: %lu su kien bi bo\n", (unsigned long)ink_bus_drop_count());
    const ink_app_t *app = app_manager_current();
    printf("App: %s, %u khoi app_alloc dang song\n", app ? app->name : "-", (unsigned)app_live_allocs());
    return 0;
}

static int cmd_heap(int argc, char **argv)
{
    ink_heap_stats_t s;
    ink_heap_get(&s);
    printf("RAM noi : trong %u B, thap nhat %u B, khoi lon nhat %u B\n", (unsigned)s.internal_free,
           (unsigned)s.internal_min_free, (unsigned)s.internal_largest);
    printf("DMA     : trong %u B\n", (unsigned)s.dma_free);
    printf("PSRAM   : trong %u B, thap nhat %u B\n", (unsigned)s.psram_free, (unsigned)s.psram_min_free);
    return 0;
}

static int cmd_tasks(int argc, char **argv)
{
    static char buf[1536];
    vTaskList(buf);
    printf("Ten             Trang thai Uu tien Stack_con So\n%s", buf);
    return 0;
}

static void print_log(const ink_log_record_t *r, void *arg)
{
    printf("  %8lu ms  %-14s %s dong %u\n", (unsigned long)r->time_ms, ink_comp_name(r->comp),
           r->level == ESP_LOG_ERROR ? "LOI " : "CANH", r->line);
}

static int cmd_log(int argc, char **argv)
{
    ink_log_foreach(print_log, NULL);
    return 0;
}

/* Bơm intent giả để chạy thử luồng app khi chưa có driver nút. Ví dụ: intent select */
static const struct {
    const char *name;
    ink_intent_t intent;
} s_intents[] = {
    { "next", INK_INTENT_NEXT },     { "back", INK_INTENT_BACK },   { "select", INK_INTENT_SELECT },
    { "menu", INK_INTENT_MENU },     { "flip", INK_INTENT_FLIP },   { "again", INK_INTENT_GRADE_AGAIN },
    { "hard", INK_INTENT_GRADE_HARD }, { "good", INK_INTENT_GRADE_GOOD }, { "easy", INK_INTENT_GRADE_EASY },
    { "audio", INK_INTENT_PLAY_AUDIO }, { "sleep", INK_INTENT_SLEEP },
};

static int cmd_intent(int argc, char **argv)
{
    if (argc == 2) {
        for (size_t i = 0; i < sizeof(s_intents) / sizeof(s_intents[0]); i++) {
            if (!strcmp(argv[1], s_intents[i].name)) {
                const ink_intent_evt_t evt = { .intent = s_intents[i].intent };
                return ink_bus_post(INK_EVT_INTENT, &evt, sizeof(evt)) == ESP_OK ? 0 : 1;
            }
        }
    }
    printf("Dung: intent <ten>. Ten:");
    for (size_t i = 0; i < sizeof(s_intents) / sizeof(s_intents[0]); i++) {
        printf(" %s", s_intents[i].name);
    }
    printf("\n");
    return 1;
}

esp_err_t console_start(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "ink>";
    esp_console_dev_usb_serial_jtag_config_t hw_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl), "console", "repl");

    const esp_console_cmd_t cmds[] = {
        { .command = "status", .help = "Tinh trang component, bus, app dang chay", .func = cmd_status },
        { .command = "heap", .help = "So lieu heap (RAM noi, DMA, PSRAM)", .func = cmd_heap },
        { .command = "tasks", .help = "Danh sach task va stack con trong", .func = cmd_tasks },
        { .command = "log", .help = "Cac loi/canh bao gan day (RAM ring buffer)", .func = cmd_log },
        { .command = "intent", .help = "Bom intent gia: intent <ten>", .func = cmd_intent },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_RETURN_ON_ERROR(esp_console_cmd_register(&cmds[i]), "console", "register");
    }
    esp_console_register_help_command();
    return esp_console_start_repl(repl);
}
