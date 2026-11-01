/* Thiết kế: docs/design/display/display.md */
#include "ink_display_internal.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_display_policy.h"
#include <string.h>

static const char *TAG = "ink_display";

#define PARTIALS_BEFORE_FULL 10
#define MAX_PARTIAL_PERCENT  100

static ink_hal_display_device_t   s_device;
static ink_display_policy_state_t s_policy; /* chỉ task màn đụng */
static size_t                     s_frame_bytes;
static uint8_t                   *s_old_plane;   /* mặt phẳng cũ dựng riêng (đẩy lại viền, khung đầu); chỉ task màn đụng */
static uint8_t                    s_edge_margin; /* bản sao edge_margin của policy; chỉ task màn đụng */
static TaskHandle_t               s_task;

/* ---- Vùng đệm 3 khung ---- */
/* Mỗi khung có đúng một chủ; đổi chủ trong s_lock. Task màn giữ khung đang hiện,
 * và thêm khung đang vẽ trong lúc vẽ, nên main mượn lúc đó thì không còn khung
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
#define NOTIFY_FRAME   (1U << 0)
#define NOTIFY_STATS   (1U << 1)
#define NOTIFY_PROFILE (1U << 2)
#define NOTIFY_EDGE    (1U << 3)
#define NOTIFY_TEMP    (1U << 4)

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

/* ---- Profile theo nhiệt độ ---- */
/* Bảng dải nhiệt độ của từng slot (display.md). Hiện có một dải 0–40 °C. */

typedef struct {
    int8_t min_celsius;
    int8_t max_celsius;
    size_t profiles[INK_HAL_DISPLAY_SLOT_COUNT];
} temperature_band_t;

#define BAND_COUNT 1

static temperature_band_t s_bands[BAND_COUNT] = {
    {.min_celsius = 0, .max_celsius = 40}
};
static size_t s_selected[INK_HAL_DISPLAY_SLOT_COUNT]; /* profile đang gán trong driver; chỉ task màn đụng */
static int8_t s_temperature;
static bool   s_temperature_valid;

/* Yêu cầu từ lệnh console, task màn lấy ra. */
static struct {
    ink_hal_display_slot_t slot;
    size_t                 profile;
    uint8_t                edge_margin;
} s_request;

static const temperature_band_t *band_for(int8_t celsius) {
    for (size_t i = 0; i < BAND_COUNT; i++) {
        if (celsius >= s_bands[i].min_celsius && celsius <= s_bands[i].max_celsius) { return &s_bands[i]; }
    }
    /* ngoài mọi dải: dùng dải gần nhất */
    return celsius < s_bands[0].min_celsius ? &s_bands[0] : &s_bands[BAND_COUNT - 1];
}

static void select_profile(ink_hal_display_slot_t slot, size_t profile) {
    if (s_selected[slot] == profile) { return; }
    const esp_err_t error = s_device.operations->select_profile(s_device.context, slot, profile);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "select_profile: %s", esp_err_to_name(error));
        return;
    }
    s_selected[slot] = profile;
}

static void read_temperature(void) {
    const esp_err_t error = s_device.operations->read_temperature(s_device.context, &s_temperature);
    s_temperature_valid   = error == ESP_OK;
    if (error != ESP_OK) { ESP_LOGW(TAG, "read_temperature: %s", esp_err_to_name(error)); }
}

/* Đọc nhiệt độ trước mỗi lần full, chọn profile của dải đó cho mọi slot. */
static void select_profiles_for_temperature(void) {
    read_temperature();
    if (!s_temperature_valid) { return; }
    const temperature_band_t *band = band_for(s_temperature);
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) { select_profile((ink_hal_display_slot_t)slot, band->profiles[slot]); }
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

static void record(const uint32_t segments_ms[SEGMENT_COUNT], ink_hal_display_slot_t slot) {
    memcpy(s_history[s_history_next], segments_ms, sizeof(s_history[0]));
    s_history_next = (s_history_next + 1) % STATS_SIZE;
    if (s_history_count < STATS_SIZE) { s_history_count++; }
    ESP_LOGI(TAG, "flip %lums %s %s: input %lu draw %lu queue %lu diff %lu spi %lu panel %lu", (unsigned long)segments_ms[SEGMENT_FLIP],
             slot == INK_HAL_DISPLAY_SLOT_FULL ? "full" : "partial", s_device.capabilities.profiles[s_selected[slot]].name,
             (unsigned long)segments_ms[SEGMENT_INPUT], (unsigned long)segments_ms[SEGMENT_DRAW], (unsigned long)segments_ms[SEGMENT_QUEUE],
             (unsigned long)segments_ms[SEGMENT_DIFF], (unsigned long)segments_ms[SEGMENT_SPI], (unsigned long)segments_ms[SEGMENT_PANEL]);
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

/* ---- Yêu cầu từ task khác (lệnh console) ---- */

bool display_is_started(void) {
    return s_task != nullptr;
}

const ink_hal_display_capabilities_t *display_capabilities(void) {
    return &s_device.capabilities;
}

size_t display_slot_profile(ink_hal_display_slot_t slot) {
    taskENTER_CRITICAL(&s_lock);
    const size_t profile = s_bands[0].profiles[slot];
    taskEXIT_CRITICAL(&s_lock);
    return profile;
}

void display_request_profile(ink_hal_display_slot_t slot, size_t profile) {
    taskENTER_CRITICAL(&s_lock);
    s_request.slot    = slot;
    s_request.profile = profile;
    taskEXIT_CRITICAL(&s_lock);
    xTaskNotify(s_task, NOTIFY_PROFILE, eSetBits);
}

void display_request_edge_margin(uint8_t margin) {
    taskENTER_CRITICAL(&s_lock);
    s_request.edge_margin = margin;
    taskEXIT_CRITICAL(&s_lock);
    xTaskNotify(s_task, NOTIFY_EDGE, eSetBits);
}

void display_request_temperature(void) {
    xTaskNotify(s_task, NOTIFY_TEMP, eSetBits);
}

void display_request_stats(void) {
    xTaskNotify(s_task, NOTIFY_STATS, eSetBits);
}

/* ---- Task màn ---- */

#define TASK_STACK_BYTES 4096 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY    5

static uint32_t elapsed_ms(int64_t from_us, int64_t to_us) {
    return (uint32_t)((to_us - from_us) / 1000);
}

/* Khung `slot` thành khung đang hiện; khung đang hiện cũ được trả về. */
static void make_shown(int slot) {
    taskENTER_CRITICAL(&s_lock);
    if (s_shown != NO_FRAME) { s_frames[s_shown].owner = OWNER_FREE; }
    taskEXIT_CRITICAL(&s_lock);
    s_shown = slot;
}

/* Mặt phẳng cũ cho lần vẽ: khung đang hiện, hoặc bản dựng riêng. */
static const uint8_t *old_plane(const uint8_t *shown, const uint8_t *next, const ink_display_policy_decision_t *decision) {
    if (shown == nullptr) {
        /* Chưa biết màn đang hiện gì: coi mọi điểm là đổi màu để panel kéo hết. */
        for (size_t i = 0; i < s_frame_bytes; i++) { s_old_plane[i] = (uint8_t)~next[i]; }
        return s_old_plane;
    }
    if (decision->mode == INK_DISPLAY_POLICY_MODE_PARTIAL && s_edge_margin > 0) {
        const ink_display_policy_rect_t window = decision->window;
        if (ink_display_policy_edge_plane(&s_policy, shown, next, window, s_old_plane) == INK_DISPLAY_POLICY_OK) { return s_old_plane; }
    }
    return shown;
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
    if (decision.mode == INK_DISPLAY_POLICY_MODE_NONE) {
        make_shown(slot);
        return;
    }
    const ink_hal_display_slot_t display_slot =
        decision.mode == INK_DISPLAY_POLICY_MODE_FULL ? INK_HAL_DISPLAY_SLOT_FULL : INK_HAL_DISPLAY_SLOT_PARTIAL;
    if (display_slot == INK_HAL_DISPLAY_SLOT_FULL) { select_profiles_for_temperature(); }
    const uint8_t *plane_old = old_plane(shown, frame->buffer, &decision);
    const int64_t  diff_us   = esp_timer_get_time();

    const ink_hal_display_rect_t window = {
        .x      = decision.window.x,
        .y      = decision.window.y,
        .width  = decision.window.width,
        .height = decision.window.height,
    };
    ink_hal_display_timing_t timing = {};
    const esp_err_t          error  = s_device.operations->draw(s_device.context, frame->buffer, plane_old, window, display_slot, &timing);
    /* Vẽ xong hay lỗi, khung mới đều là khung đang hiện; lỗi thì lần sau full. */
    make_shown(slot);
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
    record(segments_ms, display_slot);
}

static void handle_requests(uint32_t bits) {
    taskENTER_CRITICAL(&s_lock);
    const ink_hal_display_slot_t slot    = s_request.slot;
    const size_t                 profile = s_request.profile;
    const uint8_t                margin  = s_request.edge_margin;
    if (bits & NOTIFY_PROFILE) {
        for (size_t i = 0; i < BAND_COUNT; i++) { s_bands[i].profiles[slot] = profile; }
    }
    taskEXIT_CRITICAL(&s_lock);
    if (bits & NOTIFY_PROFILE) {
        select_profile(slot, profile);
        ESP_LOGI(TAG, "profile %s: %s", slot == INK_HAL_DISPLAY_SLOT_FULL ? "full" : "partial",
                 s_device.capabilities.profiles[profile].name);
    }
    if (bits & NOTIFY_EDGE) {
        if (ink_display_policy_set_edge_margin(&s_policy, margin) == INK_DISPLAY_POLICY_OK) {
            s_edge_margin = margin;
            ESP_LOGI(TAG, "edge margin %u", margin);
        } else {
            ESP_LOGE(TAG, "edge margin %u quá lớn", margin);
        }
    }
    if (bits & NOTIFY_TEMP) {
        read_temperature();
        if (s_temperature_valid) { ESP_LOGI(TAG, "nhiệt độ controller %d °C", s_temperature); }
    }
}

static void task_main(void *user_data) {
    (void)user_data;
    for (;;) {
        uint32_t bits = 0;
        xTaskNotifyWait(0, UINT32_MAX, &bits, portMAX_DELAY);
        handle_requests(bits);
        if (bits & NOTIFY_FRAME) { draw_latest(); }
        if (bits & NOTIFY_STATS) { print_stats(); }
    }
}

/* ---- Khởi động ---- */

static void free_buffers(void) {
    for (int i = 0; i < FRAME_COUNT; i++) {
        heap_caps_free(s_frames[i].buffer);
        s_frames[i].buffer = nullptr;
    }
    heap_caps_free(s_old_plane);
    s_old_plane = nullptr;
}

esp_err_t ink_display_start(const ink_hal_display_device_t *device, const uint8_t *background) {
    if (s_task != nullptr) { return ESP_ERR_INVALID_STATE; }
    ESP_RETURN_ON_FALSE(device
                            && device->operations
                            && device->operations->draw
                            && device->operations->select_profile
                            && device->operations->read_temperature,
                        ESP_ERR_INVALID_ARG, TAG, "null arg");
    const ink_hal_display_capabilities_t *capabilities = &device->capabilities;
    const ink_display_policy_config_t     config       = {
                  .width                = capabilities->width,
                  .height               = capabilities->height,
                  .x_align              = capabilities->x_align,
                  .partials_before_full = PARTIALS_BEFORE_FULL,
                  .max_partial_percent  = MAX_PARTIAL_PERCENT,
                  .edge_margin          = 0,
    };
    ESP_RETURN_ON_FALSE(ink_display_policy_init(&s_policy, &config) == INK_DISPLAY_POLICY_OK, ESP_ERR_INVALID_ARG, TAG, "capabilities");
    ESP_RETURN_ON_FALSE(capabilities->profile_count > 0, ESP_ERR_INVALID_ARG, TAG, "no profile");
    s_device      = *device;
    s_frame_bytes = ((size_t)capabilities->width + 7) / 8 * capabilities->height;
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) {
        s_selected[slot] = capabilities->default_profiles[slot];
        for (size_t i = 0; i < BAND_COUNT; i++) { s_bands[i].profiles[slot] = capabilities->default_profiles[slot]; }
    }

    for (int i = 0; i < FRAME_COUNT; i++) {
        s_frames[i] = (frame_t){.buffer = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)};
        if (s_frames[i].buffer == nullptr) { goto no_memory; }
    }
    s_old_plane = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (s_old_plane == nullptr) { goto no_memory; }
    if (background != nullptr) {
        memcpy(s_frames[0].buffer, background, s_frame_bytes);
        s_frames[0].owner = OWNER_DISPLAY;
        s_shown           = 0;
        s_latest          = 0;
        ink_display_policy_assume_shown(&s_policy);
    }
    if (xTaskCreatePinnedToCore(task_main, "ink_display", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, &s_task, tskNO_AFFINITY) != pdPASS) {
        s_task = nullptr;
        goto no_memory;
    }
    return ESP_OK;

no_memory:
    free_buffers();
    return ESP_ERR_NO_MEM;
}
