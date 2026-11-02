/* Thiết kế: docs/design/display/bench.md */

/* Slot ảnh và hai đường vẽ. Chạy trên task bench, trừ on_drawn chạy trên task màn. */
#include "bench.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ink_display_policy.h"
#include <string.h>

static const char *TAG = "bench_draw";

#define RAW_PARTIALS_BEFORE_FULL 10

static bench_path_t             s_path;
static ink_hal_display_device_t s_device;
static size_t                   s_frame_bytes;
static uint8_t                 *s_slots[BENCH_SLOT_COUNT]; /* PSRAM */

/* ---- Slot ---- */

const ink_hal_display_capabilities_t *bench_draw_capabilities(void) {
    return &s_device.capabilities;
}

size_t bench_draw_frame_bytes(void) {
    return s_frame_bytes;
}

esp_err_t bench_draw_upload(uint8_t slot, uint16_t offset, const uint8_t *data, size_t length) {
    ESP_RETURN_ON_FALSE(slot < BENCH_SLOT_COUNT && (size_t)offset + length <= s_frame_bytes, ESP_ERR_INVALID_ARG, TAG, "upload");
    memcpy(&s_slots[slot][offset], data, length);
    return ESP_OK;
}

static ink_hal_display_rect_t full_window(void) {
    return (ink_hal_display_rect_t){.width = s_device.capabilities.width, .height = s_device.capabilities.height};
}

static uint32_t elapsed_ms(int64_t from_us, int64_t to_us) {
    return (uint32_t)((to_us - from_us) / 1000);
}

/* ---- Đường raw: task bench gọi thẳng driver ---- */
/* Bộ đệm vẽ nằm trong RAM nội như hợp đồng của driver; slot nằm trong PSRAM nên chép sang trước khi vẽ. */

static uint8_t                   *s_shown; /* ảnh đang hiện */
static uint8_t                   *s_new;
static uint8_t                   *s_old;
static bool                       s_shown_valid;
static ink_display_policy_state_t s_policy;
static size_t                     s_raw_profiles[INK_HAL_DISPLAY_SLOT_COUNT]; /* profile đang gán, để ghi vào gói DRAWN */

static void invert(const uint8_t *source, uint8_t *destination) {
    for (size_t i = 0; i < s_frame_bytes; i++) { destination[i] = (uint8_t)~source[i]; }
}

/* Kiểu vẽ và window; trả false nếu policy chọn không vẽ. */
static bool choose_window(bench_kind_t kind, ink_hal_display_rect_t requested, ink_display_mode_t *out_mode,
                          ink_hal_display_rect_t *out_window) {
    if (kind == BENCH_KIND_FULL) {
        *out_mode   = INK_DISPLAY_MODE_FULL;
        *out_window = full_window();
        return true;
    }
    if (kind == BENCH_KIND_PARTIAL) {
        *out_mode   = INK_DISPLAY_MODE_PARTIAL;
        *out_window = requested.width > 0 && requested.height > 0 ? requested : full_window();
        return true;
    }

    ink_display_policy_decision_t decision;
    const uint8_t                *shown = s_shown_valid ? s_shown : nullptr;
    if (ink_display_policy_decide(&s_policy, shown, s_new, &decision) != INK_DISPLAY_POLICY_OK) { return false; }
    if (decision.mode == INK_DISPLAY_POLICY_MODE_NONE) { return false; }
    *out_mode   = decision.mode == INK_DISPLAY_POLICY_MODE_FULL ? INK_DISPLAY_MODE_FULL : INK_DISPLAY_MODE_PARTIAL;
    *out_window = (ink_hal_display_rect_t){decision.window.x, decision.window.y, decision.window.width, decision.window.height};
    return true;
}

static const uint8_t *prepare_old_plane(bench_old_source_t source, uint8_t old_slot, uint8_t edge_margin, ink_display_mode_t mode,
                                        ink_hal_display_rect_t window) {
    if (source == BENCH_OLD_SLOT && old_slot < BENCH_SLOT_COUNT) {
        memcpy(s_old, s_slots[old_slot], s_frame_bytes);
        return s_old;
    }
    if (source == BENCH_OLD_INVERTED || !s_shown_valid) {
        invert(s_new, s_old);
        return s_old;
    }

    const bool push_edges = mode == INK_DISPLAY_MODE_PARTIAL
        && edge_margin > 0
        && ink_display_policy_set_edge_margin(&s_policy, edge_margin) == INK_DISPLAY_POLICY_OK;
    const ink_display_policy_rect_t policy_window = {window.x, window.y, window.width, window.height};
    if (push_edges && ink_display_policy_edge_plane(&s_policy, s_shown, s_new, policy_window, s_old) == INK_DISPLAY_POLICY_OK) {
        return s_old;
    }
    return s_shown;
}

esp_err_t bench_draw_raw(uint8_t slot, bench_kind_t kind, bench_old_source_t old_source, uint8_t old_slot, uint8_t edge_margin,
                         ink_hal_display_rect_t window, bench_drawn_t *out) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_INVALID_STATE, TAG, "not raw");
    ESP_RETURN_ON_FALSE(slot < BENCH_SLOT_COUNT, ESP_ERR_INVALID_ARG, TAG, "slot");
    const int64_t start_us = esp_timer_get_time();
    *out                   = (bench_drawn_t){.slot = slot, .mode = INK_DISPLAY_MODE_NONE};
    memcpy(s_new, s_slots[slot], s_frame_bytes);

    ink_display_mode_t     mode;
    ink_hal_display_rect_t draw_window;
    if (!choose_window(kind, window, &mode, &draw_window)) { return ESP_OK; }
    const ink_hal_display_slot_t display_slot = mode == INK_DISPLAY_MODE_FULL ? INK_HAL_DISPLAY_SLOT_FULL : INK_HAL_DISPLAY_SLOT_PARTIAL;
    const uint8_t               *plane_old    = prepare_old_plane(old_source, old_slot, edge_margin, mode, draw_window);
    out->mode                                 = mode;
    out->window                               = draw_window;
    out->profile                              = (uint8_t)s_raw_profiles[display_slot];
    out->diff_ms                              = elapsed_ms(start_us, esp_timer_get_time());

    out->error = s_device.operations->draw(s_device.context, s_new, plane_old, draw_window, display_slot, &out->driver);
    memcpy(s_shown, s_new, s_frame_bytes);
    s_shown_valid = out->error == ESP_OK;
    if (out->error != ESP_OK) { ink_display_policy_invalidate(&s_policy); }
    return ESP_OK;
}

static esp_err_t init_raw(void) {
    s_shown = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_new   = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_old   = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(s_shown && s_new && s_old, ESP_ERR_NO_MEM, TAG, "raw buffers");

    const ink_hal_display_capabilities_t *capabilities = &s_device.capabilities;
    const ink_display_policy_config_t     config       = {
                  .width                = capabilities->width,
                  .height               = capabilities->height,
                  .x_align              = capabilities->x_align,
                  .partials_before_full = RAW_PARTIALS_BEFORE_FULL,
                  .max_partial_percent  = 100,
                  .edge_margin          = 0,
    };
    ESP_RETURN_ON_FALSE(ink_display_policy_init(&s_policy, &config) == INK_DISPLAY_POLICY_OK, ESP_ERR_INVALID_ARG, TAG, "policy");
    for (int slot = 0; slot < INK_HAL_DISPLAY_SLOT_COUNT; slot++) { s_raw_profiles[slot] = capabilities->default_profiles[slot]; }
    return ESP_OK;
}

/* ---- Đường pipeline: qua ink_display như app thật ---- */

static QueueHandle_t         s_queue;
static ink_display_options_t s_options;
static uint8_t               s_submitted_slot; /* slot giao gần nhất; ink_display luôn vẽ khung mới nhất */

/* Task màn: chép report vào message cho task bench, không làm gì lâu. */
static void on_drawn(const ink_display_report_t *report, void *user_data) {
    (void)user_data;
    const bench_event_message_t message = {.type = BENCH_EVENT_DRAWN, .report = *report};
    xQueueSend(s_queue, &message, portMAX_DELAY);
}

esp_err_t bench_draw_submit(uint8_t slot, uint32_t press_ms) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_PIPELINE, ESP_ERR_INVALID_STATE, TAG, "not pipeline");
    ESP_RETURN_ON_FALSE(slot < BENCH_SLOT_COUNT, ESP_ERR_INVALID_ARG, TAG, "slot");
    uint8_t *frame = ink_display_acquire();
    ESP_RETURN_ON_FALSE(frame != nullptr, ESP_ERR_INVALID_STATE, TAG, "acquire");
    memcpy(frame, s_slots[slot], s_frame_bytes);
    s_submitted_slot = slot;
    return ink_display_submit(frame, press_ms);
}

void bench_draw_from_report(const ink_display_report_t *report, bench_drawn_t *out) {
    const bool from_button = report->press_ms != 0;
    *out                   = (bench_drawn_t){
                          .slot                = s_submitted_slot,
                          .error               = report->error,
                          .mode                = report->mode,
                          .window              = report->window,
                          .profile             = (uint8_t)report->profile,
                          .temperature_celsius = report->temperature_celsius,
                          .temperature_valid   = report->temperature_valid,
                          .input_ms            = from_button ? (uint32_t)(report->acquire_us / 1000) - report->press_ms : 0,
                          .queue_ms            = elapsed_ms(report->submit_us, report->take_us),
                          .diff_ms             = report->mode == INK_DISPLAY_MODE_NONE ? 0 : elapsed_ms(report->take_us, report->diff_us),
                          .driver              = report->driver,
    };
}

static esp_err_t init_pipeline(void) {
    s_options                         = ink_display_default_options(&s_device.capabilities);
    const ink_display_config_t config = {
        .device   = &s_device,
        .options  = &s_options,
        .on_drawn = on_drawn,
    };
    return ink_display_start(&config);
}

/* ---- Profile, nhiệt độ ---- */

esp_err_t bench_draw_select_profile(ink_hal_display_slot_t slot, size_t profile) {
    ESP_RETURN_ON_FALSE(slot < INK_HAL_DISPLAY_SLOT_COUNT && profile < s_device.capabilities.profile_count, ESP_ERR_INVALID_ARG, TAG,
                        "profile");
    if (s_path == BENCH_PATH_PIPELINE) {
        ink_display_options_t options = s_options;
        if (slot == INK_HAL_DISPLAY_SLOT_FULL) {
            options.full_profile = profile;
        } else {
            options.partial_profile = profile;
        }
        ESP_RETURN_ON_ERROR(ink_display_set_options(&options), TAG, "set_options");
        s_options = options;
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(s_device.operations->select_profile(s_device.context, slot, profile), TAG, "select_profile");
    s_raw_profiles[slot] = profile;
    return ESP_OK;
}

esp_err_t bench_draw_read_temperature(int8_t *out_celsius) {
    /* Đường pipeline: chỉ task màn gọi driver; nhiệt độ có trong gói DRAWN của lần full. */
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_NOT_SUPPORTED, TAG, "pipeline");
    return s_device.operations->read_temperature(s_device.context, out_celsius);
}

/* ---- Khởi động ---- */

esp_err_t bench_draw_init(bench_path_t path, const ink_hal_display_device_t *device, void *event_queue) {
    s_path        = path;
    s_device      = *device;
    s_queue       = event_queue;
    s_frame_bytes = ((size_t)device->capabilities.width + 7) / 8 * device->capabilities.height;
    for (int slot = 0; slot < BENCH_SLOT_COUNT; slot++) {
        s_slots[slot] = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_SPIRAM);
        ESP_RETURN_ON_FALSE(s_slots[slot] != nullptr, ESP_ERR_NO_MEM, TAG, "slot %d", slot);
        memset(s_slots[slot], 0xFF, s_frame_bytes);
    }
    return path == BENCH_PATH_RAW ? init_raw() : init_pipeline();
}

esp_err_t bench_draw_set_options(const ink_display_options_t *options) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_PIPELINE, ESP_ERR_INVALID_STATE, TAG, "not pipeline");
    ESP_RETURN_ON_ERROR(ink_display_set_options(options), TAG, "set_options");
    s_options = *options;
    return ESP_OK;
}
