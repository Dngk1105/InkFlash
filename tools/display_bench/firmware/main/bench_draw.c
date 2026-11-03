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

#define RAW_PURPOSE 255 /* gói DRAWN của đường raw: không có mục đích */

static bench_path_t             s_path;
static ink_hal_display_device_t s_device;
static size_t                   s_frame_bytes;
static uint8_t                 *s_slots[BENCH_SLOT_COUNT]; /* PSRAM */
static ink_display_options_t    s_options;

/* ---- Slot ---- */

const ink_hal_display_capabilities_t *bench_draw_capabilities(void) {
    return &s_device.capabilities;
}

const ink_display_options_t *bench_draw_options(void) {
    return &s_options;
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

static uint8_t *s_shown; /* ảnh đang hiện */
static uint8_t *s_new;
static uint8_t *s_old;
static bool     s_shown_valid;

static void invert(const uint8_t *source, uint8_t *destination) {
    for (size_t i = 0; i < s_frame_bytes; i++) { destination[i] = (uint8_t)~source[i]; }
}

/*
 * Window theo kiểu của gói. "Vùng đổi" là vùng mà mặt phẳng cũ khác khung mới, tức đúng các điểm bảng sóng sẽ kéo:
 * mặt phẳng cũ là ảnh đang hiện thì so với ảnh đang hiện, là slot khác thì so với slot đó. Mặt phẳng cũ bằng khung
 * mới thì không điểm nào bị kéo, chỉ chạy bảng giữ màu: vẽ cả màn. Trả false nếu vùng đổi trống.
 */
static bool choose_window(const bench_raw_settings_t *settings, const uint8_t *plane_old, ink_hal_display_rect_t *out_window) {
    if (settings->window_kind == BENCH_WINDOW_GIVEN && settings->window.width > 0 && settings->window.height > 0) {
        *out_window = settings->window;
        return true;
    }
    if (settings->window_kind != BENCH_WINDOW_CHANGED || plane_old == s_new) {
        *out_window = full_window();
        return true;
    }
    const ink_hal_display_capabilities_t *capabilities = &s_device.capabilities;
    const ink_display_policy_config_t     config       = {
                  .width = capabilities->width, .height = capabilities->height, .x_align = capabilities->x_align};
    ink_display_policy_rect_t changed;
    if (!ink_display_policy_changed_window(&config, plane_old, s_new, &changed)) { return false; }
    *out_window = (ink_hal_display_rect_t){changed.x, changed.y, changed.width, changed.height};
    return true;
}

static const uint8_t *prepare_old_plane(const bench_raw_settings_t *settings) {
    switch (settings->old_source) {
        case BENCH_OLD_SLOT:
            if (settings->old_slot < BENCH_SLOT_COUNT) {
                memcpy(s_old, s_slots[settings->old_slot], s_frame_bytes);
                return s_old;
            }
            break;
        case BENCH_OLD_SAME:
            return s_new;
        case BENCH_OLD_INVERTED:
            invert(s_new, s_old);
            return s_old;
        case BENCH_OLD_SHOWN:
            break;
    }
    if (!s_shown_valid) {
        invert(s_new, s_old);
        return s_old;
    }
    return s_shown;
}

esp_err_t bench_draw_raw(uint8_t slot, const bench_raw_settings_t *settings, bench_drawn_t *out) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_INVALID_STATE, TAG, "not raw");
    ESP_RETURN_ON_FALSE(slot < BENCH_SLOT_COUNT, ESP_ERR_INVALID_ARG, TAG, "slot");
    ESP_RETURN_ON_FALSE(settings->profile < s_device.capabilities.profile_count, ESP_ERR_INVALID_ARG, TAG, "profile");
    const int64_t start_us = esp_timer_get_time();
    *out                   = (bench_drawn_t){.slot = slot, .action = INK_DISPLAY_ACTION_NONE, .frame = true, .purpose = RAW_PURPOSE};
    memcpy(s_new, s_slots[slot], s_frame_bytes);

    const uint8_t         *plane_old = prepare_old_plane(settings);
    ink_hal_display_rect_t window;
    if (!choose_window(settings, plane_old, &window)) { return ESP_OK; }
    out->action     = INK_DISPLAY_ACTION_DRAW;
    out->window     = window;
    out->profile_id = s_device.capabilities.profiles[settings->profile].id;
    out->diff_ms    = elapsed_ms(start_us, esp_timer_get_time());

    out->error = s_device.operations->draw(s_device.context, s_new, plane_old, window, settings->profile, &settings->config, &out->driver);
    memcpy(s_shown, s_new, s_frame_bytes);
    s_shown_valid = out->error == ESP_OK;
    return ESP_OK;
}

esp_err_t bench_draw_power_off(void) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_INVALID_STATE, TAG, "not raw");
    return s_device.operations->power_off(s_device.context);
}

static esp_err_t init_raw(void) {
    s_shown = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_new   = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_old   = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ESP_RETURN_ON_FALSE(s_shown && s_new && s_old, ESP_ERR_NO_MEM, TAG, "raw buffers");
    return ESP_OK;
}

/* ---- Đường pipeline: qua ink_display như app thật ---- */

static QueueHandle_t s_queue;
static uint8_t       s_submitted_slot; /* slot giao gần nhất; ink_display luôn vẽ khung mới nhất */

/* Task màn: chép report vào message cho task bench, không làm gì lâu. */
static void on_drawn(const ink_display_report_t *report, void *user_data) {
    (void)user_data;
    const bench_event_message_t message = {.type = BENCH_EVENT_DRAWN, .report = *report};
    xQueueSend(s_queue, &message, portMAX_DELAY);
}

esp_err_t bench_draw_submit(uint8_t slot, uint32_t press_ms, ink_display_intent_t intent, uint8_t flags) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_PIPELINE, ESP_ERR_INVALID_STATE, TAG, "not pipeline");
    ESP_RETURN_ON_FALSE(slot < BENCH_SLOT_COUNT, ESP_ERR_INVALID_ARG, TAG, "slot");
    uint8_t *frame = ink_display_acquire();
    ESP_RETURN_ON_FALSE(frame != nullptr, ESP_ERR_INVALID_STATE, TAG, "acquire");
    memcpy(frame, s_slots[slot], s_frame_bytes);
    s_submitted_slot = slot;
    return ink_display_submit(frame, press_ms, intent, flags);
}

void bench_draw_from_report(const ink_display_report_t *report, bench_drawn_t *out) {
    const bool from_button = report->frame && report->press_ms != 0;
    const bool drew        = report->action != INK_DISPLAY_ACTION_NONE && report->action != INK_DISPLAY_ACTION_POWER_OFF;
    *out                   = (bench_drawn_t){
                          .slot                = s_submitted_slot,
                          .error               = report->error,
                          .action              = report->action,
                          .frame               = report->frame,
                          .purpose             = report->purpose == INK_HAL_DISPLAY_PURPOSE_COUNT ? RAW_PURPOSE : (uint8_t)report->purpose,
                          .profile_id          = report->profile_id,
                          .window              = report->window,
                          .temperature_celsius = report->temperature_celsius,
                          .temperature_valid   = report->temperature_valid,
                          .max_debt            = report->max_debt,
                          .ceiling             = report->ceiling,
                          .flagged_count       = report->flagged_count,
                          .input_ms            = from_button ? (uint32_t)(report->acquire_us / 1000) - report->press_ms : 0,
                          .queue_ms            = report->frame ? elapsed_ms(report->submit_us, report->take_us) : 0,
                          .diff_ms             = drew ? elapsed_ms(report->take_us, report->diff_us) : 0,
                          .driver              = report->driver,
    };
}

static esp_err_t init_pipeline(void) {
    const ink_display_config_t config = {
        .device   = &s_device,
        .options  = &s_options,
        .on_drawn = on_drawn,
    };
    return ink_display_start(&config);
}

esp_err_t bench_draw_set_options(const ink_display_options_t *options) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_PIPELINE, ESP_ERR_INVALID_STATE, TAG, "not pipeline");
    ESP_RETURN_ON_ERROR(ink_display_set_options(options), TAG, "set_options");
    s_options = *options;
    return ESP_OK;
}

/* ---- Nhiệt độ ---- */

esp_err_t bench_draw_read_temperature(int8_t *out_celsius) {
    /* Đường pipeline: chỉ task màn gọi driver; nhiệt độ có trong gói DRAWN của lần dọn mạnh. */
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_NOT_SUPPORTED, TAG, "pipeline");
    return s_device.operations->read_temperature(s_device.context, out_celsius);
}

/* ---- Khởi động ---- */

esp_err_t bench_draw_init(bench_path_t path, const ink_hal_display_device_t *device, void *event_queue) {
    s_path        = path;
    s_device      = *device;
    s_queue       = event_queue;
    s_frame_bytes = ((size_t)device->capabilities.width + 7) / 8 * device->capabilities.height;
    s_options     = ink_display_default_options(&device->capabilities);
    for (int slot = 0; slot < BENCH_SLOT_COUNT; slot++) {
        s_slots[slot] = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_SPIRAM);
        ESP_RETURN_ON_FALSE(s_slots[slot] != nullptr, ESP_ERR_NO_MEM, TAG, "slot %d", slot);
        memset(s_slots[slot], 0xFF, s_frame_bytes);
    }
    return path == BENCH_PATH_RAW ? init_raw() : init_pipeline();
}
