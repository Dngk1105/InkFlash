#include <math.h>
#include <stdio.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_gfx.h"
#include "ink_hal_audio.h"
#include "ink_hal_display.h"
#include "ink_hal_imu.h"
#include "ink_hal_led.h"
#include "ink_log.h"
#include "ink_service.h"

/*
 * Demo benchmark: chuyển thể từ test/src/main.cpp (PlatformIO + Arduino). Mỗi hàm tương ứng một hàm gốc:
 *   runFullRefresh → run_full_refresh        playBeep → play_beep
 *   runPartialUpdate → run_partial_update    printMenu → print_menu
 *   runPartialBenchmarkSequence → run_partial_sequence
 * Chỉ dùng HAL (màn, IMU, loa, LED) nên chạy được trên mọi linh kiện, kể cả driver giả lập.
 *
 * Khác bản gốc: font 5x7 phóng to thay cho FreeMono; phím '1','2','3' gõ ở console `ink>` rồi Enter;
 * thời gian "Full/Partial" trên màn là số đo thật lần trước, không phải số cố định.
 */
#define LIVE_PERIOD_MS 400
#define LIVE_CLEAN_EVERY 25 /* cứ 25 frame làm mới toàn phần một lần để xoá bóng mờ */

static ink_fb_t s_fb;
static bool s_have_display;
static bool s_live;
static int s_frame;
static uint32_t s_last_full_ms;
static uint32_t s_last_part_ms;
static int64_t s_last_live_us;

static uint32_t elapsed_ms(int64_t start_us)
{
    return (uint32_t)((esp_timer_get_time() - start_us) / 1000);
}

static void led(uint8_t r, uint8_t g, uint8_t b)
{
    ink_led_set(r, g, b); /* không có LED thì bỏ qua */
}

/* Bíp hình sin. Biên độ đầy đủ ở đây; âm lượng 25% như demo do driver loa quyết định. */
static void play_beep(float freq, int ms)
{
    if (!ink_speaker_present()) {
        return;
    }
    const int total = INK_AUDIO_RATE_HZ * ms / 1000;
    int16_t buf[128];
    for (int idx = 0; idx < total;) {
        int chunk = total - idx < 128 ? total - idx : 128;
        for (int i = 0; i < chunk; i++) {
            float t = (float)(idx + i) / INK_AUDIO_RATE_HZ;
            buf[i] = (int16_t)(sinf(2.0f * (float)M_PI * freq * t) * 32767.0f);
        }
        size_t done = 0;
        if (ink_speaker_write(buf, chunk, &done, 1000) != ESP_OK) {
            return;
        }
        idx += chunk;
    }
}

static void read_orientation(float *pitch, float *roll, float *heading)
{
    ink_orient_t o;
    *pitch = *roll = *heading = 0;
    if (ink_imu_read_orientation(&o) == ESP_OK) {
        *pitch = o.pitch_deg;
        *roll = o.roll_deg;
        *heading = o.heading_deg;
    }
}

/* ---- Vẽ màn (bố cục cho 416x240; màn nhỏ hơn thì bị cắt) ---- */

/* Vùng cập nhật cục bộ: x 15, y 45, rộng 386, cao 180, như demo gốc. */
static ink_rect_t partial_region(void)
{
    ink_rect_t r = { 15, 45, 386, 180 };
    if (r.x + r.w > s_fb.w) {
        r.w = s_fb.w > r.x ? s_fb.w - r.x : 0;
    }
    if (r.y + r.h > s_fb.h) {
        r.h = s_fb.h > r.y ? s_fb.h - r.y : 0;
    }
    return r;
}

/* Nhãn tối đa 30 ký tự ("> " + nhãn rộng tới x=380): dài hơn sẽ tràn ra ngoài vùng partial và để lại điểm thừa. */
static void render_full(const char *label, ink_refresh_t mode)
{
    ink_fb_clear(&s_fb, INK_WHITE);
    ink_fb_text(&s_fb, 10, 8, 2, INK_BLACK, "E-PAPER REFRESH RATE TEST");
    ink_fb_hline(&s_fb, 10, 30, 396, INK_BLACK);
    ink_fb_draw_rect(&s_fb, 10, 42, 396, 188, INK_BLACK);

    ink_fb_text(&s_fb, 20, 52, 2, INK_BLACK, mode == INK_REFRESH_CLEAN ? "MODE: DEEP CLEAN" : "MODE: FULL REFRESH");
    ink_fb_printf(&s_fb, 20, 74, 2, INK_BLACK, "> %s", label);

    if (s_last_full_ms) {
        ink_fb_printf(&s_fb, 20, 102, 3, INK_BLACK, "FULL: %lu ms", (unsigned long)s_last_full_ms);
    } else {
        ink_fb_text(&s_fb, 20, 102, 3, INK_BLACK, "FULL: ...");
    }
    if (s_last_part_ms) {
        ink_fb_printf(&s_fb, 20, 136, 2, INK_BLACK, "PARTIAL: %lu ms (%.1f FPS)", (unsigned long)s_last_part_ms,
                      1000.0f / (float)s_last_part_ms);
    } else {
        ink_fb_text(&s_fb, 20, 136, 2, INK_BLACK, "PARTIAL: not measured");
    }
    ink_fb_text(&s_fb, 20, 172, 2, INK_BLACK, "BTN A: partial  BTN B: full");
    ink_fb_text(&s_fb, 20, 192, 2, INK_BLACK, "CONSOLE 1:full 2:seq 3:live");
    ink_fb_text(&s_fb, 20, 210, 2, INK_BLACK, "4:deep clean");
}

static void render_partial(int counter, float pitch, float roll, float heading)
{
    const ink_rect_t r = partial_region();
    ink_fb_fill_rect(&s_fb, r.x, r.y, r.w, r.h, INK_WHITE);

    ink_fb_printf(&s_fb, 20, 52, 2, INK_BLACK, "MODE: PARTIAL #%03d", counter);
    ink_fb_printf(&s_fb, 20, 76, 3, INK_BLACK, "FRAME #%d", counter);
    if (s_last_part_ms) {
        ink_fb_printf(&s_fb, 20, 108, 2, INK_BLACK, "Time: %lu ms (%.2f FPS)", (unsigned long)s_last_part_ms,
                      1000.0f / (float)s_last_part_ms);
    } else {
        ink_fb_text(&s_fb, 20, 108, 2, INK_BLACK, "Time: measuring...");
    }
    ink_fb_printf(&s_fb, 20, 130, 2, INK_BLACK, "P:%+6.1f R:%+6.1f deg", pitch, roll);
    ink_fb_printf(&s_fb, 20, 152, 2, INK_BLACK, "Heading: %5.1f deg", heading);

    /* Thanh tiến độ động */
    int bar_w = (counter % 20) * 18;
    ink_fb_draw_rect(&s_fb, 20, 182, 360, 12, INK_BLACK);
    ink_fb_fill_rect(&s_fb, 22, 184, bar_w, 8, INK_BLACK);
}

/* ---- Các hàm demo ---- */

/* mode = INK_REFRESH_FULL (sóng chuẩn) hoặc INK_REFRESH_CLEAN (đảo đen/trắng trước khi vẽ, sạch hẳn nhưng chậm). */
static uint32_t refresh_whole_screen(const char *label, ink_refresh_t mode)
{
    const bool clean = mode == INK_REFRESH_CLEAN;
    printf("\n[BENCHMARK] >>> BAT DAU %s <<<\n", clean ? "DEEP CLEAN (DAO DEN/TRANG)" : "FULL REFRESH (QUET TOAN PHAN)");
    led(255, 150, 0); /* cam */
    uint32_t ms = 0;
    if (s_have_display) {
        render_full(label, mode);
        int64_t start = esp_timer_get_time();
        esp_err_t err = ink_display_draw(s_fb.buf, NULL, mode);
        ms = elapsed_ms(start);
        if (err != ESP_OK) {
            printf("[BENCHMARK] -> LOI: %s\n", esp_err_to_name(err));
        } else {
            if (!clean) {
                s_last_full_ms = ms; /* CLEAN không tính vào số đo FULL */
            }
            printf("[BENCHMARK] -> %s XONG trong %lu ms (%.3f giay)!\n", clean ? "DEEP CLEAN" : "FULL REFRESH",
                   (unsigned long)ms, ms / 1000.0f);
        }
    } else {
        printf("[BENCHMARK] khong co man hinh\n");
    }
    led(0, 255, 0);
    play_beep(880.0f, 80);
    return ms;
}

static uint32_t run_full_refresh(const char *label)
{
    return refresh_whole_screen(label, INK_REFRESH_FULL);
}

static uint32_t run_deep_clean(const char *label)
{
    return refresh_whole_screen(label, INK_REFRESH_CLEAN);
}

static uint32_t run_partial_update(int counter, float pitch, float roll, float heading)
{
    led(0, 150, 255); /* xanh dương */
    uint32_t ms = 0;
    if (s_have_display) {
        render_partial(counter, pitch, roll, heading);
        const ink_rect_t region = partial_region();
        int64_t start = esp_timer_get_time();
        esp_err_t err = ink_display_draw(s_fb.buf, &region, INK_REFRESH_PARTIAL);
        ms = elapsed_ms(start);
        if (err == ESP_OK && ms) {
            s_last_part_ms = ms;
            printf("[PARTIAL #%02d] Thoi gian: %lu ms | FPS: %.2f | Pitch:%+5.1f | Roll:%+5.1f\n", counter,
                   (unsigned long)ms, 1000.0f / (float)ms, pitch, roll);
        } else if (err != ESP_OK) {
            printf("[PARTIAL #%02d] LOI: %s\n", counter, esp_err_to_name(err));
        }
    }
    led(0, 255, 0);
    return ms;
}

/* Chuỗi 10 lần partial liên tiếp để tính FPS trung bình. */
static void run_partial_sequence(void)
{
    printf("\n========================================================\n");
    printf(">>> BAT DAU CHUOI 10 LAN FAST PARTIAL REFRESH LIEN TIEP <<<\n");
    printf("  (Quan sat man hinh: khong chop den/trang, so nhay lien tuc)\n");
    printf("========================================================\n");

    uint32_t total_ms = 0;
    for (int i = 1; i <= 10; i++) {
        float p, r, h;
        read_orientation(&p, &r, &h);
        total_ms += run_partial_update(i, p, r, h);
        play_beep(1200.0f + i * 50, 25);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (total_ms) {
        float avg_ms = (float)total_ms / 10.0f;
        float avg_fps = 1000.0f / avg_ms;
        printf("\n--------------------------------------------------------\n");
        printf(">>> KET QUA BENCHMARK PARTIAL REFRESH:\n");
        printf("  - Thoi gian trung binh: %.1f ms / frame\n", avg_ms);
        printf("  - Toc do lam tuoi (FPS) : %.2f FPS\n", avg_fps);
        printf("--------------------------------------------------------\n\n");
    }
}

static void print_menu(void)
{
    printf("\n========================================================\n");
    printf("     BENCHMARK TOC DO REFRESH MAN HINH E-PAPER 3.7\"\n");
    printf("========================================================\n");
    printf("  Console `ink>` (go phim roi Enter):\n");
    printf("  1  : Do 1 lan FULL REFRESH (quet toan phan)\n");
    printf("  2  : Chay chuoi 10 lan FAST PARTIAL (khong chop)\n");
    printf("  3  : Bat/tat LIVE TRACKER (cap nhat goc IMU lien tuc)\n");
    printf("  4  : DEEP CLEAN: dao den/trang toan man roi ve (sach hang, ~9 giay)\n");
    printf("  Nut A: 1 lan Fast Partial.  Nut B: Full Refresh.\n");
    printf("========================================================\n\n");
}

/* ---- Service ---- */

static void on_start(void)
{
    ink_display_info_t info;
    if (ink_display_get_info(&info) == ESP_OK && ink_fb_init(&s_fb, info.width, info.height) == ESP_OK) {
        s_have_display = true;
    }
    led(0, 100, 255);
    print_menu();
    ink_speaker_start(); /* không có loa thì lỗi, bỏ qua */
    if (s_have_display) {
        run_full_refresh("INITIAL SCREEN READY");
    } else {
        INK_LOGW(INK_COMP_SVC_GUI, INK_LOG_EVT_STATE, "demo: khong co man hinh");
    }
}

static void on_event(const ink_svc_msg_t *msg)
{
    if (msg->id == INK_EVT_BUTTON) {
        const ink_button_evt_t *b = (const ink_button_evt_t *)msg->data;
        if (b->action != INK_BTN_PRESS) {
            return;
        }
        if (b->button == INK_BTN_A) {
            float p, r, h;
            read_orientation(&p, &r, &h);
            run_partial_update(++s_frame, p, r, h);
            play_beep(1000.0f, 40);
        } else {
            run_full_refresh("TRIGGERED BY BTN B");
        }
    } else if (msg->id == INK_EVT_DEMO_CMD) {
        switch (msg->data[0]) {
        case '1':
            run_full_refresh("MANUAL FULL REFRESH");
            break;
        case '2':
            run_partial_sequence();
            break;
        case '4':
            run_deep_clean("MANUAL DEEP CLEAN");
            break;
        case '3':
            s_live = !s_live;
            s_last_live_us = 0;
            printf(">>> [LIVE TRACKER]: %s\n", s_live ? "BAT (DANG CHAY)" : "TAT (DUNG)");
            break;
        default:
            break;
        }
    }
}

/* Live tracker: cập nhật liên tục góc nghiêng bằng partial refresh. */
static void on_tick(void)
{
    if (!s_live || (esp_timer_get_time() - s_last_live_us) < (int64_t)LIVE_PERIOD_MS * 1000) {
        return;
    }
    s_last_live_us = esp_timer_get_time();
    float p, r, h;
    read_orientation(&p, &r, &h);
    run_partial_update(++s_frame, p, r, h);
    if (s_frame % LIVE_CLEAN_EVERY == 0) {
        printf(">>> [AUTO CLEAN] Deep clean sau %d frame de xoa bong mo...\n", LIVE_CLEAN_EVERY);
        run_deep_clean("PERIODIC DEEP CLEAN");
    }
}

const ink_service_def_t svc_demo_def = {
    .name = "svc_demo",
    .comp = INK_COMP_SVC_GUI,
    .stack_size = 8192,
    .priority = 4,
    .on_start = on_start,
    .on_event = on_event,
    .tick_ms = 50,
    .on_tick = on_tick,
};
