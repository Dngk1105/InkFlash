/* Thiết kế: docs/design/display/display.md */
#include "ink_display.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_display_policy.h"
#include "ink_frame_slots.h"
#include <string.h>

static const char *TAG = "ink_display";

#define DEFAULT_PARTIALS_BEFORE_FULL 10
#define DEFAULT_EDGE_MARGIN          0
#define MAX_PARTIAL_PERCENT          100
#define TASK_STACK_BYTES             4096 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY                5

/* Thông báo cho task màn (bit của task notification). */
#define NOTIFY_FRAME   (1U << 0)
#define NOTIFY_OPTIONS (1U << 1)

/* ---- Trạng thái ---- */

typedef struct {
    uint8_t *buffer;
    uint32_t press_ms;
    int64_t  acquire_us;
    int64_t  submit_us;
} frame_t;

static ink_hal_display_device_t     s_device;
static ink_display_drawn_callback_t s_on_drawn;
static void                        *s_user_data;
static TaskHandle_t                 s_task;
static size_t                       s_frame_bytes;
static frame_t                      s_frames[INK_FRAME_SLOTS_COUNT];

/* s_lock giữ s_slots và s_requested_options: task main và task màn cùng đụng. */
static portMUX_TYPE            s_lock = portMUX_INITIALIZER_UNLOCKED;
static ink_frame_slots_state_t s_slots;
static ink_display_options_t   s_requested_options;

/* Chỉ task màn đụng. */
static ink_display_policy_state_t s_policy;
static ink_display_options_t      s_options;   /* cài đặt đang dùng */
static uint8_t                   *s_old_plane; /* mặt phẳng cũ dựng riêng (đẩy lại viền, khung đầu) */

static void lock(void) {
    taskENTER_CRITICAL(&s_lock);
}

static void unlock(void) {
    taskEXIT_CRITICAL(&s_lock);
}

/* ---- Cài đặt ---- */

ink_display_options_t ink_display_default_options(const ink_hal_display_capabilities_t *capabilities) {
    ink_display_options_t options = {
        .partials_before_full = DEFAULT_PARTIALS_BEFORE_FULL,
        .edge_margin          = DEFAULT_EDGE_MARGIN,
    };
    if (capabilities != nullptr) {
        options.full_profile    = capabilities->default_profiles[INK_HAL_DISPLAY_SLOT_FULL];
        options.partial_profile = capabilities->default_profiles[INK_HAL_DISPLAY_SLOT_PARTIAL];
    }
    return options;
}

static bool options_valid(const ink_display_options_t *options, const ink_hal_display_capabilities_t *capabilities) {
    const bool profiles_valid =
        options->full_profile < capabilities->profile_count && options->partial_profile < capabilities->profile_count;
    return profiles_valid && options->partials_before_full > 0 && options->edge_margin <= INK_DISPLAY_POLICY_MAX_EDGE_MARGIN;
}

esp_err_t ink_display_set_options(const ink_display_options_t *options) {
    if (s_task == nullptr) { return ESP_ERR_INVALID_STATE; }
    if (options == nullptr || !options_valid(options, &s_device.capabilities)) { return ESP_ERR_INVALID_ARG; }
    lock();
    s_requested_options = *options;
    unlock();
    xTaskNotify(s_task, NOTIFY_OPTIONS, eSetBits);
    return ESP_OK;
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

/* ---- Profile theo nhiệt độ (task màn) ---- */
/* Bảng dải nhiệt độ (display.md). Hiện có một dải 0–40 °C, profile lấy từ cài đặt. */

typedef struct {
    int8_t min_celsius;
    int8_t max_celsius;
    size_t profiles[INK_HAL_DISPLAY_SLOT_COUNT];
} temperature_band_t;

#define BAND_COUNT 1

static temperature_band_t s_bands[BAND_COUNT] = {
    {.min_celsius = 0, .max_celsius = 40}
};
static size_t s_selected[INK_HAL_DISPLAY_SLOT_COUNT]; /* profile đang gán trong driver */

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

/* Đọc nhiệt độ trước mỗi lần full, chọn profile của dải đó cho mọi slot. */
static void select_profiles_for_temperature(ink_display_report_t *report) {
    const esp_err_t error     = s_device.operations->read_temperature(s_device.context, &report->temperature_celsius);
    report->temperature_valid = error == ESP_OK;
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "read_temperature: %s", esp_err_to_name(error));
        return;
    }
    const temperature_band_t *band = band_for(report->temperature_celsius);
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) { select_profile((ink_hal_display_slot_t)slot, band->profiles[slot]); }
}

/* ---- Áp dụng cài đặt (task màn) ---- */

static void apply_options(const ink_display_options_t *options) {
    s_options = *options;
    for (size_t i = 0; i < BAND_COUNT; i++) {
        s_bands[i].profiles[INK_HAL_DISPLAY_SLOT_FULL]    = options->full_profile;
        s_bands[i].profiles[INK_HAL_DISPLAY_SLOT_PARTIAL] = options->partial_profile;
    }
    select_profile(INK_HAL_DISPLAY_SLOT_FULL, options->full_profile);
    select_profile(INK_HAL_DISPLAY_SLOT_PARTIAL, options->partial_profile);
    /* options đã kiểm ở ink_display_set_options(): hai hàm dưới không lỗi */
    (void)ink_display_policy_set_partials_before_full(&s_policy, options->partials_before_full);
    (void)ink_display_policy_set_edge_margin(&s_policy, options->edge_margin);
}

static void apply_requested_options(void) {
    lock();
    const ink_display_options_t options = s_requested_options;
    unlock();
    apply_options(&options);
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

    const bool push_edges = decision->mode == INK_DISPLAY_POLICY_MODE_PARTIAL && s_options.edge_margin > 0;
    if (push_edges && ink_display_policy_edge_plane(&s_policy, shown, next, decision->window, s_old_plane) == INK_DISPLAY_POLICY_OK) {
        return s_old_plane;
    }
    return shown;
}

static ink_display_mode_t report_mode(ink_display_policy_mode_t mode) {
    switch (mode) {
        case INK_DISPLAY_POLICY_MODE_PARTIAL:
            return INK_DISPLAY_MODE_PARTIAL;
        case INK_DISPLAY_POLICY_MODE_FULL:
            return INK_DISPLAY_MODE_FULL;
        case INK_DISPLAY_POLICY_MODE_NONE:
            break;
    }
    return INK_DISPLAY_MODE_NONE;
}

static void notify_drawn(const ink_display_report_t *report) {
    if (s_on_drawn != nullptr) { s_on_drawn(report, s_user_data); }
}

/* Policy chọn cách vẽ, rồi driver vẽ; report ghi lại kết quả. Trả false nếu policy lỗi (khung không vẽ). */
static bool draw_frame(const frame_t *frame, ink_display_report_t *report) {
    const uint8_t                *shown = shown_buffer();
    ink_display_policy_decision_t decision;
    if (ink_display_policy_decide(&s_policy, shown, frame->buffer, &decision) != INK_DISPLAY_POLICY_OK) {
        report->error = ESP_ERR_INVALID_STATE;
        return false;
    }
    report->mode   = report_mode(decision.mode);
    report->window = (ink_hal_display_rect_t){
        .x      = decision.window.x,
        .y      = decision.window.y,
        .width  = decision.window.width,
        .height = decision.window.height,
    };
    if (report->mode == INK_DISPLAY_MODE_NONE) { return true; }

    const ink_hal_display_slot_t slot = report->mode == INK_DISPLAY_MODE_FULL ? INK_HAL_DISPLAY_SLOT_FULL : INK_HAL_DISPLAY_SLOT_PARTIAL;
    if (slot == INK_HAL_DISPLAY_SLOT_FULL) { select_profiles_for_temperature(report); }
    report->profile          = s_selected[slot];
    const uint8_t *plane_old = prepare_old_plane(shown, frame->buffer, &decision);
    report->diff_us          = esp_timer_get_time();
    report->error            = s_device.operations->draw(s_device.context, frame->buffer, plane_old, report->window, slot, &report->driver);
    return true;
}

static void draw_latest(void) {
    const int8_t index = take_frame();
    if (index == INK_FRAME_SLOTS_NONE) { return; }
    const frame_t       *frame  = &s_frames[index];
    ink_display_report_t report = {
        .press_ms   = frame->press_ms,
        .acquire_us = frame->acquire_us,
        .submit_us  = frame->submit_us,
        .take_us    = esp_timer_get_time(),
    };

    if (!draw_frame(frame, &report)) {
        ESP_LOGE(TAG, "policy decide");
        finish_frame(index, false);
    } else {
        /* Vẽ xong hay lỗi, khung mới đều là khung đang hiện; lỗi thì lần sau full. */
        finish_frame(index, true);
        if (report.error != ESP_OK) {
            ESP_LOGE(TAG, "draw: %s", esp_err_to_name(report.error));
            ink_display_policy_invalidate(&s_policy);
        }
    }
    notify_drawn(&report);
}

static void task_main(void *user_data) {
    (void)user_data;
    for (;;) {
        uint32_t bits = 0;
        xTaskNotifyWait(0, UINT32_MAX, &bits, portMAX_DELAY);
        if (bits & NOTIFY_OPTIONS) { apply_requested_options(); }
        if (bits & NOTIFY_FRAME) { draw_latest(); }
    }
}

/* ---- Khởi động ---- */

static esp_err_t check_config(const ink_display_config_t *config) {
    ESP_RETURN_ON_FALSE(config != nullptr && config->device != nullptr && config->device->operations != nullptr, ESP_ERR_INVALID_ARG, TAG,
                        "null device");
    const ink_hal_display_operations_t *operations = config->device->operations;
    ESP_RETURN_ON_FALSE(operations->draw && operations->select_profile && operations->read_temperature, ESP_ERR_INVALID_ARG, TAG,
                        "null operation");
    ESP_RETURN_ON_FALSE(config->device->capabilities.profile_count > 0, ESP_ERR_INVALID_ARG, TAG, "no profile");
    const bool options_ok = config->options == nullptr || options_valid(config->options, &config->device->capabilities);
    ESP_RETURN_ON_FALSE(options_ok, ESP_ERR_INVALID_ARG, TAG, "options");
    return ESP_OK;
}

static esp_err_t init_policy(const ink_hal_display_capabilities_t *capabilities, const ink_display_options_t *options) {
    const ink_display_policy_config_t config = {
        .width                = capabilities->width,
        .height               = capabilities->height,
        .x_align              = capabilities->x_align,
        .partials_before_full = options->partials_before_full,
        .max_partial_percent  = MAX_PARTIAL_PERCENT,
        .edge_margin          = options->edge_margin,
    };
    const bool valid = ink_display_policy_init(&s_policy, &config) == INK_DISPLAY_POLICY_OK;
    ESP_RETURN_ON_FALSE(valid, ESP_ERR_INVALID_ARG, TAG, "capabilities");
    return ESP_OK;
}

/* Profile driver đang dùng là mặc định của nó; cài đặt khác thì gán lại. */
static void init_profiles(const ink_hal_display_capabilities_t *capabilities, const ink_display_options_t *options) {
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) { s_selected[slot] = capabilities->default_profiles[slot]; }
    apply_options(options);
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

esp_err_t ink_display_start(const ink_display_config_t *config) {
    if (s_task != nullptr) { return ESP_ERR_INVALID_STATE; }
    ESP_RETURN_ON_ERROR(check_config(config), TAG, "config");
    const ink_hal_display_capabilities_t *capabilities = &config->device->capabilities;
    const ink_display_options_t options = config->options != nullptr ? *config->options : ink_display_default_options(capabilities);
    ESP_RETURN_ON_ERROR(init_policy(capabilities, &options), TAG, "policy");

    s_device      = *config->device;
    s_on_drawn    = config->on_drawn;
    s_user_data   = config->user_data;
    s_frame_bytes = ((size_t)capabilities->width + 7) / 8 * capabilities->height;
    init_profiles(capabilities, &options);
    if (ink_frame_slots_init(&s_slots) != INK_FRAME_SLOTS_OK) { return ESP_ERR_INVALID_STATE; }

    ESP_RETURN_ON_ERROR(allocate_buffers(), TAG, "buffers");
    if (config->background != nullptr) { restore_background(config->background); }

    const BaseType_t created =
        xTaskCreatePinnedToCore(task_main, "ink_display", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, &s_task, tskNO_AFFINITY);
    if (created != pdPASS) {
        s_task = nullptr;
        free_buffers();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
