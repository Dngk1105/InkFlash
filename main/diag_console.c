#include <stdio.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "drv_buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drv_imu.h"
#include "ink_bus.h"
#include "ink_hal_audio.h"
#include "ink_hal_display.h"
#include "ink_hal_imu.h"
#include "ink_hal_power.h"
#include "esp_check.h"
#include "esp_console.h"
#include "diag_console.h"
#include "ink_device.h"
#include "ink_health.h"
#include "ink_log.h"

static void print_device(const ink_device_t *dev, void *arg)
{
    printf("  %-12s %-9s fails=%lu%s\n", dev->name, ink_health_str(ink_health_get(dev->comp)),
           (unsigned long)ink_health_fail_count(dev->comp), dev->optional ? " (optional)" : "");
}

static int cmd_status(int argc, char **argv)
{
    printf("Components:\n");
    for (int c = 0; c < INK_COMP_COUNT; c++) {
        ink_health_t h = ink_health_get(c);
        if (h != INK_HEALTH_UNKNOWN) {
            printf("  %-12s %s\n", ink_comp_name(c), ink_health_str(h));
        }
    }
    printf("Devices:\n");
    ink_device_foreach(print_device, NULL);
    return 0;
}

static void print_record(const ink_log_record_t *rec, void *arg)
{
    printf("  %8lu ms  %-12s lvl=%u evt=%u\n", (unsigned long)rec->time_ms,
           ink_comp_name(rec->comp), rec->level, rec->evt);
}

static int cmd_log(int argc, char **argv)
{
    ink_log_foreach(print_record, NULL);
    return 0;
}

static int cmd_imu(int argc, char **argv)
{
#if CONFIG_INK_IMU_FAKE
    if (argc == 4) { /* imu <x> <y> <z>: đặt giá trị giả lập (mg) */
        imu_fake_set((int16_t)atoi(argv[1]), (int16_t)atoi(argv[2]), (int16_t)atoi(argv[3]));
    }
#endif
    ink_accel_t a;
    esp_err_t err = ink_imu_read_accel(&a);
    if (err != ESP_OK) {
        printf("imu: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("imu: x=%d y=%d z=%d mg\n", a.x_mg, a.y_mg, a.z_mg);
    return 0;
}

/*
 * Dò dây nút bấm: quét các chân chưa dùng (kéo lên nội), báo chân nào xuống mức thấp trong 60 giây.
 * Chân của nút A/B (GPIO8, GPIO46) chỉ đọc mức, không cấu hình lại để khỏi tắt ngắt của driver.
 * Chỉ dùng để chẩn đoán dây trên breadboard.
 */
static int cmd_pinscan(int argc, char **argv)
{
    static const int free_pins[] = { 0, 1, 2, 3, 13, 14, 21, 38, 39, 40, 41, 42, 43, 44, 45, 47 };
    static const int watched[] = { 8, 46 };
    const int nfree = sizeof(free_pins) / sizeof(free_pins[0]);
    const int nwatch = sizeof(watched) / sizeof(watched[0]);
    int count[sizeof(free_pins) / sizeof(free_pins[0]) + sizeof(watched) / sizeof(watched[0])] = { 0 };
    int last[sizeof(count) / sizeof(count[0])];

    for (int i = 0; i < nfree; i++) {
        const gpio_config_t cfg = { .pin_bit_mask = 1ULL << free_pins[i], .mode = GPIO_MODE_INPUT,
                                    .pull_up_en = GPIO_PULLUP_ENABLE };
        gpio_config(&cfg);
    }
    printf("pinscan: 60 giay. Bam nut B, roi cham day GPIO46 xuong GND.\n");
    for (int i = 0; i < nfree + nwatch; i++) {
        last[i] = 1;
    }
    for (int t = 0; t < 12000; t++) { /* 12000 x 5 ms = 60 s */
        for (int i = 0; i < nfree + nwatch; i++) {
            int pin = i < nfree ? free_pins[i] : watched[i - nfree];
            int lvl = gpio_get_level(pin);
            if (lvl == 0 && last[i] == 1) {
                count[i]++;
                printf("pinscan: GPIO%d xuong thap (lan %d)\n", pin, count[i]);
            }
            last[i] = lvl;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    for (int i = 0; i < nfree; i++) {
        gpio_reset_pin(free_pins[i]);
    }
    printf("pinscan: xong.\n");
    return 0;
}

static int cmd_btn(int argc, char **argv)
{
    drv_buttons_dump();
    return 0;
}

/* Mọi khối phần cứng, nhìn qua HAL: thấy gì là thấy đúng thứ logic nhìn thấy. */
static int cmd_hw(int argc, char **argv)
{
    ink_display_info_t d;
    if (ink_display_get_info(&d) == ESP_OK) {
        printf("display : %ux%u partial=%d\n", d.width, d.height, d.partial_refresh);
    } else {
        printf("display : none\n");
    }
    printf("imu     : %s\n", ink_imu_present() ? "yes" : "none");
    printf("mic     : %s\n", ink_mic_present() ? "yes" : "none");
    printf("speaker : %s\n", ink_speaker_present() ? "yes" : "none");
    uint32_t mv;
    uint8_t pct;
    if (ink_power_battery_mv(&mv) == ESP_OK && ink_power_battery_percent(&pct) == ESP_OK) {
        printf("battery : %lu mV (%u%%)\n", (unsigned long)mv, pct);
    } else {
        printf("battery : none\n");
    }
    return 0;
}

#if CONFIG_INK_DEMO
/* Phím 1/2/3 của demo gốc: gõ phím rồi Enter. Chuyển thành event để svc_demo xử lý trong task của nó. */
static int cmd_demo_key(int argc, char **argv)
{
    uint8_t key = (uint8_t)argv[0][0];
    return ink_bus_post(INK_EVT_DEMO_CMD, &key, sizeof(key)) == ESP_OK ? 0 : 1;
}
#endif

esp_err_t diag_console_start(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "ink>";
    esp_console_dev_usb_serial_jtag_config_t hw_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl), "diag", "repl");

    const esp_console_cmd_t cmds[] = {
        { .command = "status", .help = "Trạng thái health và danh sách thiết bị", .func = cmd_status },
        { .command = "log", .help = "In RAM ring buffer log", .func = cmd_log },
        { .command = "hw", .help = "Phần cứng nhìn qua HAL: màn, imu, mic, loa, pin", .func = cmd_hw },
        { .command = "pinscan", .help = "Dò dây nút: báo chân GPIO nào xuống mức thấp trong 60 giây", .func = cmd_pinscan },
        { .command = "btn", .help = "Chẩn đoán nút bấm: mức điện thật, số lần ngắt, số event", .func = cmd_btn },
        { .command = "imu", .help = "Đọc gia tốc (mg). Driver giả lập: imu <x> <y> <z> để đặt giá trị", .func = cmd_imu },
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_RETURN_ON_ERROR(esp_console_cmd_register(&cmds[i]), "diag", "register");
    }
#if CONFIG_INK_DEMO
    const esp_console_cmd_t demo_cmds[] = {
        { .command = "1", .help = "Demo: đo 1 lần FULL REFRESH", .func = cmd_demo_key },
        { .command = "2", .help = "Demo: chuỗi 10 lần PARTIAL REFRESH", .func = cmd_demo_key },
        { .command = "3", .help = "Demo: bật/tắt LIVE TRACKER (góc IMU)", .func = cmd_demo_key },
        { .command = "4", .help = "Demo: DEEP CLEAN, đảo đen/trắng toàn màn rồi vẽ (~9 s)", .func = cmd_demo_key },
    };
    for (size_t i = 0; i < sizeof(demo_cmds) / sizeof(demo_cmds[0]); i++) {
        ESP_RETURN_ON_ERROR(esp_console_cmd_register(&demo_cmds[i]), "diag", "register");
    }
#endif
    esp_console_register_help_command();
    return esp_console_start_repl(repl);
}
