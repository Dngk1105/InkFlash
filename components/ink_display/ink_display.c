/* Thiết kế: docs/design/display/display.md */
#include "ink_display_internal.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_display_policy.h"
#include "ink_frame_slots.h"
#include <inttypes.h>
#include <string.h>

static const char *TAG = "ink_display";

#define PARTIALS_BEFORE_FULL 10
#define MAX_PARTIAL_PERCENT  100
#define TASK_STACK_BYTES     4096 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY        5

/* Thông báo cho task màn (bit của task notification). */
#define NOTIFY_FRAME   (1U << 0)
#define NOTIFY_STATS   (1U << 1)
#define NOTIFY_PROFILE (1U << 2)
#define NOTIFY_EDGE    (1U << 3)
#define NOTIFY_TEMP    (1U << 4)

/* ---- Trạng thái ---- */

typedef struct {
    uint8_t *buffer;
    uint32_t press_ms;
    int64_t  acquire_us;
    int64_t  submit_us;
} frame_t;

static ink_hal_display_device_t s_device;
static TaskHandle_t             s_task;
static size_t                   s_frame_bytes;
static frame_t                  s_frames[INK_FRAME_SLOTS_COUNT];

/* s_lock giữ s_slots, s_request và s_bands: task main, task REPL và task màn cùng đụng. */
static portMUX_TYPE            s_lock = portMUX_INITIALIZER_UNLOCKED;
static ink_frame_slots_state_t s_slots;

/* Chỉ task màn đụng. */
static ink_display_policy_state_t s_policy;
static uint8_t                   *s_old_plane;   /* mặt phẳng cũ dựng riêng (đẩy lại viền, khung đầu) */
static uint8_t                    s_edge_margin; /* bản sao edge_margin của policy */

static void lock(void) {
    taskENTER_CRITICAL(&s_lock);
}

static void unlock(void) {
    taskEXIT_CRITICAL(&s_lock);
}

/* ---- Mượn và giao khung (task main) ---- */

uint8_t *ink_display_acquire(void) {
    if (s_task == nullptr) { return nullptr; }
    ink_frame_slots_lease_t lease;
    lock();
    const ink_frame_slots_error_t error = ink_frame_slots_acquire(&s_slots, &lease);
    unlock();
    if (error != INK_FRAME_SLOTS_OK) { return nullptr; }

    frame_t *frame    = &s_frames[lease.frame];
    frame->acquire_us = esp_timer_get_time();
    /* Chép ngoài lock: khung copy_from chỉ được trả về trống khi đã có khung mới hơn được giao. */
    if (lease.fill == INK_FRAME_SLOTS_FILL_WHITE) {
        memset(frame->buffer, 0xFF, s_frame_bytes);
    } else if (lease.fill == INK_FRAME_SLOTS_FILL_COPY) {
        memcpy(frame->buffer, s_frames[lease.copy_from].buffer, s_frame_bytes);
    }
    return frame->buffer;
}

esp_err_t ink_display_submit(uint8_t *buffer, uint32_t press_ms) {
    if (s_task == nullptr) { return ESP_ERR_INVALID_STATE; }
    lock();
    const int8_t borrowed = ink_frame_slots_borrowed(&s_slots);
    unlock();
    if (borrowed == INK_FRAME_SLOTS_NONE) { return ESP_ERR_INVALID_STATE; }
    frame_t *frame = &s_frames[borrowed];
    if (buffer != frame->buffer) { return ESP_ERR_INVALID_ARG; }

    frame->press_ms  = press_ms;
    frame->submit_us = esp_timer_get_time();
    lock();
    const ink_frame_slots_error_t error = ink_frame_slots_submit(&s_slots);
    unlock();
    if (error != INK_FRAME_SLOTS_OK) { return ESP_ERR_INVALID_STATE; }
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

    lock();
    const temperature_band_t band = *band_for(s_temperature);
    unlock();
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) { select_profile((ink_hal_display_slot_t)slot, band.profiles[slot]); }
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

/* Mốc thời gian của một lần vẽ trên task màn. */
typedef struct {
    int64_t                  take_us; /* lấy khung ra khỏi hàng chờ */
    int64_t                  diff_us; /* policy và mặt phẳng cũ xong */
    ink_hal_display_timing_t driver;
} draw_timing_t;

static uint32_t s_history[STATS_SIZE][SEGMENT_COUNT]; /* ms; chỉ task màn đụng */
static uint32_t s_history_count;
static uint32_t s_history_next;

static uint32_t elapsed_ms(int64_t from_us, int64_t to_us) {
    return (uint32_t)((to_us - from_us) / 1000);
}

static void measure_segments(const frame_t *frame, const draw_timing_t *timing, uint32_t out_ms[SEGMENT_COUNT]) {
    out_ms[SEGMENT_INPUT] = (uint32_t)(frame->acquire_us / 1000) - frame->press_ms;
    out_ms[SEGMENT_DRAW]  = elapsed_ms(frame->acquire_us, frame->submit_us);
    out_ms[SEGMENT_QUEUE] = elapsed_ms(frame->submit_us, timing->take_us);
    out_ms[SEGMENT_DIFF]  = elapsed_ms(timing->take_us, timing->diff_us);
    out_ms[SEGMENT_SPI]   = timing->driver.spi_us / 1000;
    out_ms[SEGMENT_PANEL] = timing->driver.panel_us / 1000;
    out_ms[SEGMENT_FLIP]  = 0;
    for (int segment = 0; segment < SEGMENT_FLIP; segment++) { out_ms[SEGMENT_FLIP] += out_ms[segment]; }
}

static void record(const frame_t *frame, const draw_timing_t *timing, ink_hal_display_slot_t slot) {
    uint32_t *ms = s_history[s_history_next];
    measure_segments(frame, timing, ms);
    s_history_next = (s_history_next + 1) % STATS_SIZE;
    if (s_history_count < STATS_SIZE) { s_history_count++; }

    const char *slot_name    = slot == INK_HAL_DISPLAY_SLOT_FULL ? "full" : "partial";
    const char *profile_name = s_device.capabilities.profiles[s_selected[slot]].name;
    ESP_LOGI(TAG,
             "flip %" PRIu32 "ms %s %s: input %" PRIu32 " draw %" PRIu32 " queue %" PRIu32 " diff %" PRIu32 " spi %" PRIu32
             " panel %" PRIu32,
             ms[SEGMENT_FLIP], slot_name, profile_name, ms[SEGMENT_INPUT], ms[SEGMENT_DRAW], ms[SEGMENT_QUEUE], ms[SEGMENT_DIFF],
             ms[SEGMENT_SPI], ms[SEGMENT_PANEL]);
}

static void print_stats(void) {
    lock();
    const uint32_t dropped = ink_frame_slots_dropped(&s_slots);
    unlock();
    ESP_LOGI(TAG, "stats: last %" PRIu32 " flips, %" PRIu32 " frames dropped (ms: avg / min / max)", s_history_count, dropped);
    if (s_history_count == 0) { return; }

    for (int segment = 0; segment < SEGMENT_COUNT; segment++) {
        uint32_t sum = 0, low = UINT32_MAX, high = 0;
        for (uint32_t i = 0; i < s_history_count; i++) {
            const uint32_t value = s_history[i][segment];
            sum += value;
            if (value < low) { low = value; }
            if (value > high) { high = value; }
        }
        ESP_LOGI(TAG, "  %-5s %5" PRIu32 " %5" PRIu32 " %5" PRIu32, SEGMENT_NAMES[segment], sum / s_history_count, low, high);
    }
}

/* ---- Yêu cầu từ task khác (lệnh console) ---- */

/* Task màn lấy ra khi nhận bit NOTIFY tương ứng. */
static struct {
    ink_hal_display_slot_t slot;
    size_t                 profile;
    uint8_t                edge_margin;
} s_request;

bool display_is_started(void) {
    return s_task != nullptr;
}

const ink_hal_display_capabilities_t *display_capabilities(void) {
    return &s_device.capabilities;
}

size_t display_slot_profile(ink_hal_display_slot_t slot) {
    lock();
    const size_t profile = s_bands[0].profiles[slot];
    unlock();
    return profile;
}

void display_request_profile(ink_hal_display_slot_t slot, size_t profile) {
    lock();
    s_request.slot    = slot;
    s_request.profile = profile;
    unlock();
    xTaskNotify(s_task, NOTIFY_PROFILE, eSetBits);
}

void display_request_edge_margin(uint8_t margin) {
    lock();
    s_request.edge_margin = margin;
    unlock();
    xTaskNotify(s_task, NOTIFY_EDGE, eSetBits);
}

void display_request_temperature(void) {
    xTaskNotify(s_task, NOTIFY_TEMP, eSetBits);
}

void display_request_stats(void) {
    xTaskNotify(s_task, NOTIFY_STATS, eSetBits);
}

/* ---- Một lần vẽ (task màn) ---- */

/* Ảnh đang hiện trên màn, hoặc nullptr khi chưa biết. */
static const uint8_t *shown_buffer(void) {
    lock();
    const int8_t shown = ink_frame_slots_shown(&s_slots);
    unlock();
    return shown == INK_FRAME_SLOTS_NONE ? nullptr : s_frames[shown].buffer;
}

static int8_t take_frame(void) {
    lock();
    const int8_t frame = ink_frame_slots_take(&s_slots);
    unlock();
    return frame;
}

/* Khung vừa lấy ra thành khung đang hiện (`show`), hoặc trả về trống. */
static void finish_frame(int8_t frame, bool show) {
    lock();
    const ink_frame_slots_error_t error = show ? ink_frame_slots_show(&s_slots, frame) : ink_frame_slots_release(&s_slots, frame);
    unlock();
    if (error != INK_FRAME_SLOTS_OK) { ESP_LOGE(TAG, "frame_slots %s: %d", show ? "show" : "release", error); }
}

/* Mặt phẳng cũ cho lần vẽ: khung đang hiện, hoặc bản dựng riêng. */
static const uint8_t *prepare_old_plane(const uint8_t *shown, const uint8_t *next, const ink_display_policy_decision_t *decision) {
    if (shown == nullptr) {
        /* Chưa biết màn đang hiện gì: coi mọi điểm là đổi màu để panel kéo hết. */
        for (size_t i = 0; i < s_frame_bytes; i++) { s_old_plane[i] = (uint8_t)~next[i]; }
        return s_old_plane;
    }

    const bool push_edges = decision->mode == INK_DISPLAY_POLICY_MODE_PARTIAL && s_edge_margin > 0;
    if (push_edges && ink_display_policy_edge_plane(&s_policy, shown, next, decision->window, s_old_plane) == INK_DISPLAY_POLICY_OK) {
        return s_old_plane;
    }
    return shown;
}

static esp_err_t draw_planes(const uint8_t *plane_new, const uint8_t *plane_old, const ink_display_policy_decision_t *decision,
                             ink_hal_display_slot_t slot, ink_hal_display_timing_t *out_timing) {
    const ink_hal_display_rect_t window = {
        .x      = decision->window.x,
        .y      = decision->window.y,
        .width  = decision->window.width,
        .height = decision->window.height,
    };

    *out_timing = (ink_hal_display_timing_t){};
    return s_device.operations->draw(s_device.context, plane_new, plane_old, window, slot, out_timing);
}

static void draw_latest(void) {
    const int8_t index = take_frame();
    if (index == INK_FRAME_SLOTS_NONE) { return; }
    const frame_t *frame  = &s_frames[index];
    draw_timing_t  timing = {.take_us = esp_timer_get_time()};

    const uint8_t                *shown = shown_buffer();
    ink_display_policy_decision_t decision;
    if (ink_display_policy_decide(&s_policy, shown, frame->buffer, &decision) != INK_DISPLAY_POLICY_OK) {
        ESP_LOGE(TAG, "policy decide");
        finish_frame(index, false);
        return;
    }
    if (decision.mode == INK_DISPLAY_POLICY_MODE_NONE) {
        finish_frame(index, true);
        return;
    }

    const bool                   full = decision.mode == INK_DISPLAY_POLICY_MODE_FULL;
    const ink_hal_display_slot_t slot = full ? INK_HAL_DISPLAY_SLOT_FULL : INK_HAL_DISPLAY_SLOT_PARTIAL;
    if (full) { select_profiles_for_temperature(); }
    const uint8_t *plane_old = prepare_old_plane(shown, frame->buffer, &decision);
    timing.diff_us           = esp_timer_get_time();

    const esp_err_t error = draw_planes(frame->buffer, plane_old, &decision, slot, &timing.driver);
    /* Vẽ xong hay lỗi, khung mới đều là khung đang hiện; lỗi thì lần sau full. */
    finish_frame(index, true);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "draw: %s", esp_err_to_name(error));
        ink_display_policy_invalidate(&s_policy);
        return;
    }
    record(frame, &timing, slot);
}

/* ---- Xử lý yêu cầu (task màn) ---- */

static void apply_profile_request(ink_hal_display_slot_t slot, size_t profile) {
    lock();
    for (size_t i = 0; i < BAND_COUNT; i++) { s_bands[i].profiles[slot] = profile; }
    unlock();
    select_profile(slot, profile);
    ESP_LOGI(TAG, "profile %s: %s", slot == INK_HAL_DISPLAY_SLOT_FULL ? "full" : "partial", s_device.capabilities.profiles[profile].name);
}

static void apply_edge_request(uint8_t margin) {
    if (ink_display_policy_set_edge_margin(&s_policy, margin) != INK_DISPLAY_POLICY_OK) {
        ESP_LOGE(TAG, "edge margin %u quá lớn", margin);
        return;
    }
    s_edge_margin = margin;
    ESP_LOGI(TAG, "edge margin %u", margin);
}

static void report_temperature(void) {
    read_temperature();
    if (s_temperature_valid) { ESP_LOGI(TAG, "nhiệt độ controller %d °C", s_temperature); }
}

static void handle_requests(uint32_t bits) {
    lock();
    const ink_hal_display_slot_t slot    = s_request.slot;
    const size_t                 profile = s_request.profile;
    const uint8_t                margin  = s_request.edge_margin;
    unlock();

    if (bits & NOTIFY_PROFILE) { apply_profile_request(slot, profile); }
    if (bits & NOTIFY_EDGE) { apply_edge_request(margin); }
    if (bits & NOTIFY_TEMP) { report_temperature(); }
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

static esp_err_t check_device(const ink_hal_display_device_t *device) {
    ESP_RETURN_ON_FALSE(device != nullptr && device->operations != nullptr, ESP_ERR_INVALID_ARG, TAG, "null device");
    const ink_hal_display_operations_t *operations = device->operations;
    ESP_RETURN_ON_FALSE(operations->draw && operations->select_profile && operations->read_temperature, ESP_ERR_INVALID_ARG, TAG,
                        "null operation");
    ESP_RETURN_ON_FALSE(device->capabilities.profile_count > 0, ESP_ERR_INVALID_ARG, TAG, "no profile");
    return ESP_OK;
}

static esp_err_t init_policy(const ink_hal_display_capabilities_t *capabilities) {
    const ink_display_policy_config_t config = {
        .width                = capabilities->width,
        .height               = capabilities->height,
        .x_align              = capabilities->x_align,
        .partials_before_full = PARTIALS_BEFORE_FULL,
        .max_partial_percent  = MAX_PARTIAL_PERCENT,
        .edge_margin          = 0,
    };
    const bool valid = ink_display_policy_init(&s_policy, &config) == INK_DISPLAY_POLICY_OK;
    ESP_RETURN_ON_FALSE(valid, ESP_ERR_INVALID_ARG, TAG, "capabilities");
    return ESP_OK;
}

static void init_profiles(const ink_hal_display_capabilities_t *capabilities) {
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) {
        s_selected[slot] = capabilities->default_profiles[slot];
        for (size_t i = 0; i < BAND_COUNT; i++) { s_bands[i].profiles[slot] = capabilities->default_profiles[slot]; }
    }
}

static void free_buffers(void) {
    for (int i = 0; i < INK_FRAME_SLOTS_COUNT; i++) {
        heap_caps_free(s_frames[i].buffer);
        s_frames[i].buffer = nullptr;
    }
    heap_caps_free(s_old_plane);
    s_old_plane = nullptr;
}

/* 3 khung và mặt phẳng cũ trong RAM nội: driver gửi chúng qua SPI DMA. */
static esp_err_t allocate_buffers(void) {
    for (int i = 0; i < INK_FRAME_SLOTS_COUNT; i++) {
        s_frames[i] = (frame_t){.buffer = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)};
    }
    s_old_plane = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    bool allocated = s_old_plane != nullptr;
    for (int i = 0; i < INK_FRAME_SLOTS_COUNT; i++) { allocated = allocated && s_frames[i].buffer != nullptr; }
    if (!allocated) {
        free_buffers();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/* Màn nền đã vẽ trước khi ngủ thành khung 0, coi là khung đang hiện. */
static void restore_background(const uint8_t *background) {
    memcpy(s_frames[0].buffer, background, s_frame_bytes);
    if (ink_frame_slots_assume_shown(&s_slots, 0) != INK_FRAME_SLOTS_OK) { ESP_LOGE(TAG, "frame_slots assume_shown"); }
    ink_display_policy_assume_shown(&s_policy);
}

esp_err_t ink_display_start(const ink_hal_display_device_t *device, const uint8_t *background) {
    if (s_task != nullptr) { return ESP_ERR_INVALID_STATE; }
    ESP_RETURN_ON_ERROR(check_device(device), TAG, "device");
    ESP_RETURN_ON_ERROR(init_policy(&device->capabilities), TAG, "policy");
    s_device      = *device;
    s_frame_bytes = ((size_t)device->capabilities.width + 7) / 8 * device->capabilities.height;
    init_profiles(&device->capabilities);
    if (ink_frame_slots_init(&s_slots) != INK_FRAME_SLOTS_OK) { return ESP_ERR_INVALID_STATE; }

    ESP_RETURN_ON_ERROR(allocate_buffers(), TAG, "buffers");
    if (background != nullptr) { restore_background(background); }

    const BaseType_t created =
        xTaskCreatePinnedToCore(task_main, "ink_display", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, &s_task, tskNO_AFFINITY);
    if (created != pdPASS) {
        s_task = nullptr;
        free_buffers();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
