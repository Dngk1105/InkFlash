/* Thiết kế: docs/design/display-pipeline.md */
#include "ink_display.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_display_policy.h"
#include "ink_platform.h"
#include <string.h>

static const char *TAG = "ink_display";

#define PARTIALS_BEFORE_FULL 10
#define MAX_PARTIAL_PERCENT  100

static ink_hal_display_device_t   s_device;
static ink_display_policy_state_t s_policy; /* chỉ task màn đụng */
static size_t                     s_frame_bytes;
static TaskHandle_t               s_task;

/* ---- Vùng đệm 3 khung ---- */
/* Mỗi khung có đúng một chủ; đổi chủ trong s_lock. Task màn giữ 2 khung lúc so
 * (khung đang hiện và khung mới), nên main mượn lúc đó thì không còn khung
 * trống: main lấy lại khung trong hàng chờ, khung đó coi như bị bỏ. */

#define FRAME_COUNT 3
#define NO_FRAME    (-1)

typedef enum : uint8_t {
    OWNER_FREE,
    OWNER_MAIN,
    OWNER_PENDING,
    OWNER_DISPLAY,
} owner_t;

typedef struct {
    uint8_t *buffer;
    owner_t  owner;
    uint32_t press_ms;
    int64_t  acquire_us;
    int64_t  submit_us;
} frame_t;

static frame_t      s_frames[FRAME_COUNT];
static portMUX_TYPE s_lock    = portMUX_INITIALIZER_UNLOCKED;
static int          s_main    = NO_FRAME;
static int          s_pending = NO_FRAME;
static int          s_latest  = NO_FRAME; /* khung giao gần nhất; acquire chép từ đây */
static int          s_shown   = NO_FRAME; /* khung đang hiện; chỉ task màn đụng */
static uint32_t     s_dropped;

/* Thông báo cho task màn (bit của task notification). */
#define NOTIFY_FRAME (1U << 0)
#define NOTIFY_STATS (1U << 1)

uint8_t *ink_display_acquire(void) {
    if (s_task == nullptr) { return nullptr; }
    int  slot      = NO_FRAME;
    bool reclaimed = false;
    taskENTER_CRITICAL(&s_lock);
    if (s_main == NO_FRAME) {
        for (int i = 0; i < FRAME_COUNT; i++) {
            if (s_frames[i].owner == OWNER_FREE) {
                slot = i;
                break;
            }
        }
        if (slot == NO_FRAME) {
            slot      = s_pending; /* luôn có: task màn giữ tối đa 2 khung */
            s_pending = NO_FRAME;
            s_dropped++;
            reclaimed = true;
        }
        s_frames[slot].owner = OWNER_MAIN;
        s_main               = slot;
    }
    const int latest = s_latest;
    taskEXIT_CRITICAL(&s_lock);
    if (slot == NO_FRAME) { return nullptr; }

    frame_t *frame    = &s_frames[slot];
    frame->acquire_us = esp_timer_get_time();
    /* Khung `latest` chỉ được trả về trống sau khi có khung mới hơn được giao,
     * nên chép ngoài lock vẫn an toàn. */
    if (!reclaimed) {
        if (latest == NO_FRAME) {
            memset(frame->buffer, 0xFF, s_frame_bytes);
        } else {
            memcpy(frame->buffer, s_frames[latest].buffer, s_frame_bytes);
        }
    }
    return frame->buffer;
}

esp_err_t ink_display_submit(uint8_t *frame, uint32_t press_ms) {
    if (s_task == nullptr || s_main == NO_FRAME) { return ESP_ERR_INVALID_STATE; }
    frame_t *slot = &s_frames[s_main];
    if (frame != slot->buffer) { return ESP_ERR_INVALID_ARG; }
    slot->press_ms  = press_ms;
    slot->submit_us = esp_timer_get_time();

    taskENTER_CRITICAL(&s_lock);
    if (s_pending != NO_FRAME) {
        s_frames[s_pending].owner = OWNER_FREE;
        s_dropped++;
    }
    slot->owner = OWNER_PENDING;
    s_pending   = s_main;
    s_latest    = s_main;
    s_main      = NO_FRAME;
    taskEXIT_CRITICAL(&s_lock);
    xTaskNotify(s_task, NOTIFY_FRAME, eSetBits);
    return ESP_OK;
}

/* ---- Đo ---- */

#define STATS_SIZE 50

typedef enum : uint8_t {
    SEGMENT_INPUT, /* bấm → mượn khung */
    SEGMENT_DRAW,  /* mượn → giao */
    SEGMENT_QUEUE, /* giao → task màn lấy */
    SEGMENT_DIFF,
    SEGMENT_SPI,
    SEGMENT_PANEL,
    SEGMENT_FLIP, /* tổng 6 đoạn */
    SEGMENT_COUNT,
} segment_t;

static const char *const SEGMENT_NAMES[SEGMENT_COUNT] = {"input", "draw", "queue", "diff", "spi", "panel", "flip"};

static uint32_t s_history[STATS_SIZE][SEGMENT_COUNT]; /* ms; chỉ task màn đụng */
static uint32_t s_history_count;
static uint32_t s_history_next;

static void record(const uint32_t segments_ms[SEGMENT_COUNT], ink_hal_display_mode_t mode) {
    memcpy(s_history[s_history_next], segments_ms, sizeof(s_history[0]));
    s_history_next = (s_history_next + 1) % STATS_SIZE;
    if (s_history_count < STATS_SIZE) { s_history_count++; }
    ESP_LOGI(TAG, "flip %lums %s: input %lu draw %lu queue %lu diff %lu spi %lu panel %lu", (unsigned long)segments_ms[SEGMENT_FLIP],
             mode == INK_HAL_DISPLAY_MODE_FULL ? "full" : "partial", (unsigned long)segments_ms[SEGMENT_INPUT],
             (unsigned long)segments_ms[SEGMENT_DRAW], (unsigned long)segments_ms[SEGMENT_QUEUE], (unsigned long)segments_ms[SEGMENT_DIFF],
             (unsigned long)segments_ms[SEGMENT_SPI], (unsigned long)segments_ms[SEGMENT_PANEL]);
}

static void print_stats(void) {
    taskENTER_CRITICAL(&s_lock);
    const uint32_t dropped = s_dropped;
    taskEXIT_CRITICAL(&s_lock);
    ESP_LOGI(TAG, "stats: last %lu flips, %lu frames dropped (ms: avg / min / max)", (unsigned long)s_history_count,
             (unsigned long)dropped);
    if (s_history_count == 0) { return; }
    for (int segment = 0; segment < SEGMENT_COUNT; segment++) {
        uint32_t sum = 0, low = UINT32_MAX, high = 0;
        for (uint32_t i = 0; i < s_history_count; i++) {
            const uint32_t value = s_history[i][segment];
            sum += value;
            if (value < low) { low = value; }
            if (value > high) { high = value; }
        }
        ESP_LOGI(TAG, "  %-5s %5lu %5lu %5lu", SEGMENT_NAMES[segment], (unsigned long)(sum / s_history_count), (unsigned long)low,
                 (unsigned long)high);
    }
}

static int on_stats_command(int argc, char **argv) {
    (void)argc;
    (void)argv;
    xTaskNotify(s_task, NOTIFY_STATS, eSetBits);
    return 0;
}

esp_err_t ink_display_add_console_command(void) {
    if (s_task == nullptr) { return ESP_ERR_INVALID_STATE; }
    return ink_platform_console_add("display", "Thời gian vẽ: display stats", "stats", on_stats_command);
}

/* ---- Task màn ---- */

#define TASK_STACK_BYTES 4096 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY    5

static uint32_t elapsed_ms(int64_t from_us, int64_t to_us) {
    return (uint32_t)((to_us - from_us) / 1000);
}

static void draw_latest(void) {
    taskENTER_CRITICAL(&s_lock);
    const int slot = s_pending;
    if (slot != NO_FRAME) { s_frames[slot].owner = OWNER_DISPLAY; }
    s_pending = NO_FRAME;
    taskEXIT_CRITICAL(&s_lock);
    if (slot == NO_FRAME) { return; }
    const frame_t *frame   = &s_frames[slot];
    const int64_t  take_us = esp_timer_get_time();

    ink_display_policy_decision_t decision;
    const uint8_t                *shown = s_shown == NO_FRAME ? nullptr : s_frames[s_shown].buffer;
    if (ink_display_policy_decide(&s_policy, shown, frame->buffer, &decision) != INK_DISPLAY_POLICY_OK) {
        ESP_LOGE(TAG, "policy decide");
        taskENTER_CRITICAL(&s_lock);
        s_frames[slot].owner = OWNER_FREE;
        taskEXIT_CRITICAL(&s_lock);
        return;
    }
    const int64_t diff_us = esp_timer_get_time();
    /* Driver giữ ảnh cũ trong controller nên khung đang hiện hết cần ngay sau khi
     * so. */
    taskENTER_CRITICAL(&s_lock);
    if (s_shown != NO_FRAME) { s_frames[s_shown].owner = OWNER_FREE; }
    taskEXIT_CRITICAL(&s_lock);
    s_shown = slot;
    if (decision.mode == INK_DISPLAY_POLICY_MODE_NONE) { return; }

    const ink_hal_display_mode_t mode =
        decision.mode == INK_DISPLAY_POLICY_MODE_FULL ? INK_HAL_DISPLAY_MODE_FULL : INK_HAL_DISPLAY_MODE_PARTIAL;
    const ink_hal_display_rect_t window = {
        .x      = decision.window.x,
        .y      = decision.window.y,
        .width  = decision.window.width,
        .height = decision.window.height,
    };
    ink_hal_display_timing_t timing = {};
    const esp_err_t          error  = s_device.operations->draw(s_device.context, frame->buffer, window, mode, &timing);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "draw: %s", esp_err_to_name(error));
        ink_display_policy_invalidate(&s_policy);
        return;
    }

    uint32_t segments_ms[SEGMENT_COUNT] = {
        [SEGMENT_INPUT] = (uint32_t)(frame->acquire_us / 1000) - frame->press_ms,
        [SEGMENT_DRAW]  = elapsed_ms(frame->acquire_us, frame->submit_us),
        [SEGMENT_QUEUE] = elapsed_ms(frame->submit_us, take_us),
        [SEGMENT_DIFF]  = elapsed_ms(take_us, diff_us),
        [SEGMENT_SPI]   = timing.spi_us / 1000,
        [SEGMENT_PANEL] = timing.panel_us / 1000,
    };
    for (int segment = 0; segment < SEGMENT_FLIP; segment++) { segments_ms[SEGMENT_FLIP] += segments_ms[segment]; }
    record(segments_ms, mode);
}

static void task_main(void *user_data) {
    (void)user_data;
    for (;;) {
        uint32_t bits = 0;
        xTaskNotifyWait(0, UINT32_MAX, &bits, portMAX_DELAY);
        if (bits & NOTIFY_FRAME) { draw_latest(); }
        if (bits & NOTIFY_STATS) { print_stats(); }
    }
}

/* ---- Khởi động ---- */

esp_err_t ink_display_start(const ink_hal_display_device_t *device) {
    if (s_task != nullptr) { return ESP_ERR_INVALID_STATE; }
    ESP_RETURN_ON_FALSE(device && device->operations && device->operations->draw, ESP_ERR_INVALID_ARG, TAG, "null arg");
    const ink_hal_display_capabilities_t *capabilities = &device->capabilities;
    const ink_display_policy_config_t     config       = {
                  .width                = capabilities->width,
                  .height               = capabilities->height,
                  .has_partial          = capabilities->has_partial,
                  .x_align              = capabilities->partial_x_align,
                  .partials_before_full = PARTIALS_BEFORE_FULL,
                  .max_partial_percent  = MAX_PARTIAL_PERCENT,
    };
    ESP_RETURN_ON_FALSE(ink_display_policy_init(&s_policy, &config) == INK_DISPLAY_POLICY_OK, ESP_ERR_INVALID_ARG, TAG, "capabilities");
    s_device      = *device;
    s_frame_bytes = ((size_t)capabilities->width + 7) / 8 * capabilities->height;

    for (int i = 0; i < FRAME_COUNT; i++) {
        s_frames[i] = (frame_t){.buffer = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)};
        if (s_frames[i].buffer == nullptr) { goto no_memory; }
    }
    if (xTaskCreatePinnedToCore(task_main, "ink_display", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, &s_task, tskNO_AFFINITY) != pdPASS) {
        s_task = nullptr;
        goto no_memory;
    }
    return ESP_OK;

no_memory:
    for (int i = 0; i < FRAME_COUNT; i++) {
        heap_caps_free(s_frames[i].buffer);
        s_frames[i].buffer = nullptr;
    }
    return ESP_ERR_NO_MEM;
}
