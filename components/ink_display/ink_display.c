/* Design: docs/design/display/display.md */
#include "ink_display.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_frame_slots.h"
#include <string.h>

static const char *TAG = "ink_display";

/* Default options (display.md, options and report section).
 * REVIEW(display): defaults are not measured yet; tune them with the bench firmware. */
#define DEFAULT_WEIGHT_TEXT      1
#define DEFAULT_WEIGHT_UI        2
#define DEFAULT_WEIGHT_PAGE      1
#define DEFAULT_AUTO_UI_PERCENT  10
#define DEFAULT_DEBT_LIGHT       2
#define DEFAULT_DEBT_PAGE        6
#define DEFAULT_DEBT_CEILING     8
#define DEFAULT_DEBT_CEILING_MAX 15
#define DEFAULT_ACTIVE_MS        2000
#define DEFAULT_EDGE_MS          2000
#define DEFAULT_LIGHT_MS         10000
#define DEFAULT_STRONG_MS        60000
#define DEFAULT_POWER_OFF_MS     4000

#define DEFAULT_CELSIUS     25    /* sensor not read yet */
#define MIN_TRUSTED_CELSIUS (-10) /* readings outside this range are treated as sensor errors */
#define MAX_TRUSTED_CELSIUS 60
#define TASK_STACK_BYTES    4096 /* stack in internal RAM (xTaskCreatePinnedToCore default) */
#define TASK_PRIORITY       5

/* Notifications to the display task (task notification bits). */
#define NOTIFY_FRAME   (1U << 0)
#define NOTIFY_OPTIONS (1U << 1)

/* ---- State ---- */

typedef struct {
    uint8_t             *buffer;
    uint32_t             input_ms;
    ink_display_intent_t intent;
    uint8_t              flags;
    int64_t              acquire_us;
    int64_t              submit_us;
} frame_t;

static ink_hal_display_device_t      s_device;
static ink_display_report_callback_t s_on_drawn;
static void                         *s_user_data;
static TaskHandle_t                  s_task;
static size_t                        s_frame_bytes;
static frame_t                       s_frames[INK_FRAME_SLOTS_COUNT];

/* s_lock guards s_slots and s_requested_options: both the main task and the display task touch them. */
static portMUX_TYPE            s_lock = portMUX_INITIALIZER_UNLOCKED;
static ink_frame_slots_state_t s_slots;
static ink_display_options_t   s_requested_options;

/* Display task only. */
static ink_display_policy_state_t s_policy;
static ink_display_options_t      s_options;   /* options in effect */
static uint8_t                   *s_old_plane; /* separate old plane (cleanup) */
static uint8_t                   *s_pixels;    /* policy pixel_state, PSRAM */
static int8_t                     s_celsius = DEFAULT_CELSIUS;
static bool                       s_power_on;      /* drawn since the last analog power off */
static uint32_t                   s_last_drive_ms; /* latest DRF */

static void lock(void) {
    taskENTER_CRITICAL(&s_lock);
}

static void unlock(void) {
    taskEXIT_CRITICAL(&s_lock);
}

static uint32_t now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/* ---- Options ---- */

static ink_display_preset_t default_preset(const ink_hal_display_purpose_default_t *purpose_default, uint8_t weight, bool marks_edges) {
    ink_display_preset_t preset = {.config = purpose_default->config, .debt_weight = weight, .marks_edges = marks_edges};
    preset.band_count = purpose_default->band_count < INK_HAL_DISPLAY_MAX_BANDS ? purpose_default->band_count : INK_HAL_DISPLAY_MAX_BANDS;
    memcpy(preset.bands, purpose_default->bands, preset.band_count * sizeof(preset.bands[0]));
    return preset;
}

ink_display_options_t ink_display_default_options(const ink_hal_display_capabilities_t *capabilities) {
    ink_display_options_t options = {
        .auto_ui_percent  = DEFAULT_AUTO_UI_PERCENT,
        .debt_light       = DEFAULT_DEBT_LIGHT,
        .debt_page        = DEFAULT_DEBT_PAGE,
        .debt_ceiling     = DEFAULT_DEBT_CEILING,
        .debt_ceiling_max = DEFAULT_DEBT_CEILING_MAX,
        .active_ms        = DEFAULT_ACTIVE_MS,
        .edge_ms          = DEFAULT_EDGE_MS,
        .light_ms         = DEFAULT_LIGHT_MS,
        .strong_ms        = DEFAULT_STRONG_MS,
        .power_off_ms     = DEFAULT_POWER_OFF_MS,
    };
    if (capabilities == nullptr || capabilities->purpose_defaults == nullptr) { return options; }
    static const uint8_t debt_weights[INK_HAL_DISPLAY_PURPOSE_COUNT] = {
        [INK_HAL_DISPLAY_PURPOSE_TEXT] = DEFAULT_WEIGHT_TEXT,
        [INK_HAL_DISPLAY_PURPOSE_UI]   = DEFAULT_WEIGHT_UI,
        [INK_HAL_DISPLAY_PURPOSE_PAGE] = DEFAULT_WEIGHT_PAGE,
    };
    for (int purpose = 0; purpose < INK_HAL_DISPLAY_PURPOSE_COUNT; purpose++) {
        /* UI only: a text page sets thousands of edge flags, and the edge center cleanup then speckles the whole page */
        const bool marks_edges   = purpose == INK_HAL_DISPLAY_PURPOSE_UI;
        options.presets[purpose] = default_preset(&capabilities->purpose_defaults[purpose], debt_weights[purpose], marks_edges);
    }
    return options;
}

static ink_display_policy_config_t policy_config(const ink_hal_display_capabilities_t *capabilities, const ink_display_options_t *options) {
    return (ink_display_policy_config_t){
        .width   = capabilities->width,
        .height  = capabilities->height,
        .x_align = capabilities->x_align,
        .debt_weights =
            {
                           [INK_DISPLAY_POLICY_INTENT_TEXT] = options->presets[INK_HAL_DISPLAY_PURPOSE_TEXT].debt_weight,
                           [INK_DISPLAY_POLICY_INTENT_UI]   = options->presets[INK_HAL_DISPLAY_PURPOSE_UI].debt_weight,
                           [INK_DISPLAY_POLICY_INTENT_PAGE] = options->presets[INK_HAL_DISPLAY_PURPOSE_PAGE].debt_weight,
                           },
        .marks_edges =
            {
                           [INK_DISPLAY_POLICY_INTENT_TEXT] = options->presets[INK_HAL_DISPLAY_PURPOSE_TEXT].marks_edges,
                           [INK_DISPLAY_POLICY_INTENT_UI]   = options->presets[INK_HAL_DISPLAY_PURPOSE_UI].marks_edges,
                           [INK_DISPLAY_POLICY_INTENT_PAGE] = options->presets[INK_HAL_DISPLAY_PURPOSE_PAGE].marks_edges,
                           },
        .auto_ui_percent  = options->auto_ui_percent,
        .debt_light       = options->debt_light,
        .debt_page        = options->debt_page,
        .debt_ceiling     = options->debt_ceiling,
        .debt_ceiling_max = options->debt_ceiling_max,
        .active_ms        = options->active_ms,
        .edge_ms          = options->edge_ms,
        .light_ms         = options->light_ms,
        .strong_ms        = options->strong_ms,
    };
}

/* Try the policy config on a scratch state: same checks as when it is applied. */
static bool options_valid(const ink_display_options_t *options, const ink_hal_display_capabilities_t *capabilities) {
    for (int purpose = 0; purpose < INK_HAL_DISPLAY_PURPOSE_COUNT; purpose++) {
        const uint8_t count = options->presets[purpose].band_count;
        if (count == 0 || count > INK_HAL_DISPLAY_MAX_BANDS) { return false; }
    }
    static uint8_t                    probe_pixel;
    ink_display_policy_state_t        probe;
    const ink_display_policy_config_t config = policy_config(capabilities, options);
    ink_display_policy_config_t       one    = config;
    one.width                                = 1;
    one.height                               = 1;
    return config.width > 0 && config.height > 0 && ink_display_policy_init(&probe, &one, &probe_pixel) == INK_DISPLAY_POLICY_OK;
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

/* ---- Acquire and submit (main task) ---- */

uint8_t *ink_display_acquire(void) {
    if (s_task == nullptr) { return nullptr; }
    ink_frame_slots_lease_t lease;
    lock();
    const ink_frame_slots_error_t error = ink_frame_slots_acquire(&s_slots, &lease);
    unlock();
    if (error != INK_FRAME_SLOTS_OK) { return nullptr; }

    frame_t *frame    = &s_frames[lease.frame];
    frame->acquire_us = esp_timer_get_time();
    /* Copy outside the lock: frame copy_from becomes free only after a newer frame is submitted. */
    if (lease.fill == INK_FRAME_SLOTS_FILL_WHITE) {
        memset(frame->buffer, 0xFF, s_frame_bytes);
    } else if (lease.fill == INK_FRAME_SLOTS_FILL_COPY) {
        memcpy(frame->buffer, s_frames[lease.copy_from].buffer, s_frame_bytes);
    }
    return frame->buffer;
}

esp_err_t ink_display_submit(uint8_t *buffer, uint32_t input_ms, ink_display_intent_t intent, uint8_t flags) {
    if (s_task == nullptr) { return ESP_ERR_INVALID_STATE; }
    if (intent >= INK_DISPLAY_POLICY_INTENT_COUNT) { return ESP_ERR_INVALID_ARG; }
    lock();
    const int8_t acquired = ink_frame_slots_acquired(&s_slots);
    unlock();
    if (acquired == INK_FRAME_SLOTS_NONE) { return ESP_ERR_INVALID_STATE; }
    frame_t *frame = &s_frames[acquired];
    if (buffer != frame->buffer) { return ESP_ERR_INVALID_ARG; }

    frame->input_ms  = input_ms;
    frame->intent    = intent;
    frame->flags     = flags;
    frame->submit_us = esp_timer_get_time();
    lock();
    const ink_frame_slots_error_t error = ink_frame_slots_submit(&s_slots);
    unlock();
    if (error != INK_FRAME_SLOTS_OK) { return ESP_ERR_INVALID_STATE; }
    xTaskNotify(s_task, NOTIFY_FRAME, eSetBits);
    return ESP_OK;
}

/* ---- Profile from preset and temperature (display task) ---- */

static bool band_profile(const ink_hal_display_band_t *bands, uint8_t count, int8_t celsius, uint16_t *out_id) {
    if (count == 0) { return false; }
    for (uint8_t i = 0; i < count; i++) {
        if (celsius <= bands[i].max_celsius) {
            *out_id = bands[i].profile_id;
            return true;
        }
    }
    *out_id = bands[count - 1].profile_id; /* above the last band: use the last band */
    return true;
}

static bool index_of(uint16_t id, size_t *out_index) {
    const ink_hal_display_capabilities_t *capabilities = &s_device.capabilities;
    for (size_t i = 0; i < capabilities->profile_count; i++) {
        if (capabilities->profiles[i].id == id) {
            *out_index = i;
            return true;
        }
    }
    return false;
}

/* Profile index for the purpose at the current temperature; unknown profile id -> driver default. */
static size_t profile_for(ink_hal_display_purpose_t purpose, uint16_t *out_id) {
    const ink_display_preset_t *preset = &s_options.presets[purpose];
    size_t                      index  = 0;
    if (band_profile(preset->bands, preset->band_count, s_celsius, out_id) && index_of(*out_id, &index)) { return index; }
    const ink_hal_display_purpose_default_t *fallback = &s_device.capabilities.purpose_defaults[purpose];
    if (band_profile(fallback->bands, fallback->band_count, s_celsius, out_id) && index_of(*out_id, &index)) { return index; }
    *out_id = s_device.capabilities.profiles[0].id;
    return 0;
}

static void read_temperature(ink_display_report_t *report) {
    const esp_err_t error     = s_device.operations->read_temperature(s_device.context, &report->temperature_celsius);
    report->temperature_valid = error == ESP_OK;
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "read_temperature: %s", esp_err_to_name(error));
        return;
    }
    /* a value outside the panel operating range is a bad reading; not used to pick a profile */
    if (report->temperature_celsius < MIN_TRUSTED_CELSIUS || report->temperature_celsius > MAX_TRUSTED_CELSIUS) {
        ESP_LOGW(TAG, "temperature %d °C out of trusted range, keeping %d °C", report->temperature_celsius, s_celsius);
        return;
    }
    s_celsius = report->temperature_celsius;
}

/* ---- Apply options (display task) ---- */

static void apply_options(const ink_display_options_t *options) {
    s_options                                = *options;
    const ink_display_policy_config_t config = policy_config(&s_device.capabilities, options);
    /* options already checked in ink_display_set_options() */
    if (ink_display_policy_set_config(&s_policy, &config) != INK_DISPLAY_POLICY_OK) { ESP_LOGE(TAG, "policy config"); }
}

static void apply_requested_options(void) {
    lock();
    const ink_display_options_t options = s_requested_options;
    unlock();
    apply_options(&options);
}

/* ---- Frames (display task) ---- */

/* Image on the panel, or NULL when unknown. */
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

/* The taken frame becomes the shown frame (`show`), or is released. */
static void finish_frame(int8_t frame, bool show) {
    lock();
    const ink_frame_slots_error_t error = show ? ink_frame_slots_show(&s_slots, frame) : ink_frame_slots_release(&s_slots, frame);
    unlock();
    if (error != INK_FRAME_SLOTS_OK) { ESP_LOGE(TAG, "frame_slots %s: %d", show ? "show" : "release", error); }
}

static void invert_into(const uint8_t *source, uint8_t *destination) {
    for (size_t i = 0; i < s_frame_bytes; i++) { destination[i] = (uint8_t)~source[i]; }
}

static ink_hal_display_rect_t hal_rect(ink_display_policy_rect_t rect) {
    return (ink_hal_display_rect_t){.x = rect.x, .y = rect.y, .width = rect.width, .height = rect.height};
}

static ink_hal_display_purpose_t purpose_of(ink_display_policy_action_t action, ink_display_policy_intent_t intent) {
    switch (action) {
        case INK_DISPLAY_POLICY_ACTION_DRAW:
            if (intent == INK_DISPLAY_POLICY_INTENT_UI) { return INK_HAL_DISPLAY_PURPOSE_UI; }
            return intent == INK_DISPLAY_POLICY_INTENT_PAGE ? INK_HAL_DISPLAY_PURPOSE_PAGE : INK_HAL_DISPLAY_PURPOSE_TEXT;
        case INK_DISPLAY_POLICY_ACTION_CLEANUP_STRONG:
            return INK_HAL_DISPLAY_PURPOSE_CLEANUP_STRONG;
        case INK_DISPLAY_POLICY_ACTION_CLEANUP_FAST:
            return INK_HAL_DISPLAY_PURPOSE_CLEANUP_FAST;
        case INK_DISPLAY_POLICY_ACTION_CLEANUP_EDGE_CENTER:
            return INK_HAL_DISPLAY_PURPOSE_CLEANUP_EDGE_CENTER;
        case INK_DISPLAY_POLICY_ACTION_CLEANUP_EDGE_NEIGHBOR:
            return INK_HAL_DISPLAY_PURPOSE_CLEANUP_EDGE_NEIGHBOR;
        case INK_DISPLAY_POLICY_ACTION_CLEANUP_LIGHT:
            return INK_HAL_DISPLAY_PURPOSE_CLEANUP_LIGHT;
        case INK_DISPLAY_POLICY_ACTION_NONE:
            break;
    }
    return INK_HAL_DISPLAY_PURPOSE_COUNT;
}

static void fill_policy_stats(ink_display_report_t *report) {
    report->max_debt      = ink_display_policy_max_debt(&s_policy);
    report->ceiling       = ink_display_policy_ceiling(&s_policy);
    report->flagged_count = ink_display_policy_flagged_count(&s_policy);
}

static void notify_drawn(ink_display_report_t *report) {
    fill_policy_stats(report);
    if (s_on_drawn != nullptr) { s_on_drawn(report, s_user_data); }
}

/* Read the temperature if needed, pick the profile from the purpose preset, call the driver. */
static esp_err_t drive(ink_hal_display_purpose_t purpose, const uint8_t *plane_new, const uint8_t *plane_old, ink_hal_display_rect_t window,
                       ink_display_report_t *report) {
    const bool whole_clean = purpose == INK_HAL_DISPLAY_PURPOSE_CLEANUP_STRONG || purpose == INK_HAL_DISPLAY_PURPOSE_CLEANUP_FAST;
    if (whole_clean) { read_temperature(report); }
    report->purpose       = purpose;
    report->window        = window;
    const size_t index    = profile_for(purpose, &report->profile_id);
    report->decide_us     = esp_timer_get_time();
    const esp_err_t error = s_device.operations->draw(s_device.context, plane_new, plane_old, window, index,
                                                      &s_options.presets[purpose].config, &report->driver);
    s_power_on            = true;
    s_last_drive_ms       = now_ms();
    return error;
}

static void power_off(ink_display_report_t *report) {
    report->action  = INK_DISPLAY_ACTION_POWER_OFF;
    report->purpose = INK_HAL_DISPLAY_PURPOSE_COUNT;
    report->error   = s_device.operations->power_off(s_device.context);
    s_power_on      = false;
    if (report->error != ESP_OK) { ESP_LOGE(TAG, "power_off: %s", esp_err_to_name(report->error)); }
}

/* Policy decides, then the driver draws; the report records the result. Returns false on a policy error (frame not drawn). */
static bool draw_frame(const frame_t *frame, ink_display_report_t *report) {
    const uint8_t                     *shown   = shown_buffer();
    const ink_display_policy_request_t request = {.intent = frame->intent, .flags = frame->flags};
    ink_display_policy_decision_t      decision;
    if (ink_display_policy_decide(&s_policy, shown, frame->buffer, request, now_ms(), &decision) != INK_DISPLAY_POLICY_OK) {
        report->error = ESP_ERR_INVALID_STATE;
        return false;
    }
    report->action  = (ink_display_action_t)decision.action;
    report->purpose = INK_HAL_DISPLAY_PURPOSE_COUNT;
    if (decision.action == INK_DISPLAY_POLICY_ACTION_NONE) { return true; }

    const uint8_t *plane_old = shown;
    if (decision.old_plane == INK_DISPLAY_POLICY_OLD_INVERTED || shown == nullptr) {
        invert_into(frame->buffer, s_old_plane);
        plane_old = s_old_plane;
    }
    report->error = drive(purpose_of(decision.action, decision.intent), frame->buffer, plane_old, hal_rect(decision.window), report);
    return true;
}

static void draw_latest(void) {
    const int8_t index = take_frame();
    if (index == INK_FRAME_SLOTS_NONE) { return; }
    const frame_t       *frame  = &s_frames[index];
    ink_display_report_t report = {
        .is_frame   = true,
        .input_ms   = frame->input_ms,
        .acquire_us = frame->acquire_us,
        .submit_us  = frame->submit_us,
        .take_us    = esp_timer_get_time(),
    };

    if (!draw_frame(frame, &report)) {
        ESP_LOGE(TAG, "policy decide");
        finish_frame(index, false);
    } else {
        /* Drawn or failed, the new frame becomes the shown frame; on failure the next decide is cleanup strong. */
        finish_frame(index, true);
        if (report.error != ESP_OK) {
            ESP_LOGE(TAG, "draw: %s", esp_err_to_name(report.error));
            ink_display_policy_invalidate(&s_policy);
        }
    }
    notify_drawn(&report);
    if ((frame->flags & INK_DISPLAY_FLAG_BEFORE_SLEEP) != 0) {
        ink_display_report_t off = {.take_us = esp_timer_get_time()};
        power_off(&off);
        notify_drawn(&off);
    }
}

/* ---- Idle cleanup stages and power off (display task) ---- */

/* One cleanup: new plane = shown frame, old plane = shown frame with the cleaned pixels inverted. */
static void run_cleanup(ink_display_policy_action_t action) {
    const uint8_t *shown = shown_buffer();
    if (shown == nullptr) { return; }
    ink_display_report_t      report = {.action = (ink_display_action_t)action, .take_us = esp_timer_get_time()};
    ink_display_policy_rect_t window;
    if (ink_display_policy_cleanup_plane(&s_policy, shown, action, s_old_plane, &window) != INK_DISPLAY_POLICY_OK) {
        ESP_LOGE(TAG, "cleanup plane");
        return;
    }
    if (window.width > 0 && window.height > 0) {
        report.error = drive(purpose_of(action, INK_DISPLAY_POLICY_INTENT_AUTO), shown, s_old_plane, hal_rect(window), &report);
    }
    if (report.error != ESP_OK) {
        ESP_LOGE(TAG, "cleanup: %s", esp_err_to_name(report.error));
        ink_display_policy_invalidate(&s_policy);
    } else if (ink_display_policy_cleanup_done(&s_policy, action) != INK_DISPLAY_POLICY_OK) {
        ESP_LOGE(TAG, "cleanup done");
    }
    if (window.width > 0) { notify_drawn(&report); }
}

static uint32_t power_off_wait_ms(uint32_t now) {
    if (!s_power_on || s_options.power_off_ms == 0) { return INK_DISPLAY_POLICY_WAIT_FOREVER; }
    const uint32_t idle_ms = now - s_last_drive_ms;
    return idle_ms >= s_options.power_off_ms ? 0 : s_options.power_off_ms - idle_ms;
}

/* Run one due stage (cleanup or power off); returns ms until the next stage. */
static uint32_t run_idle(void) {
    const uint32_t            now = now_ms();
    ink_display_policy_idle_t idle;
    if (ink_display_policy_idle(&s_policy, now, &idle) != INK_DISPLAY_POLICY_OK) { return INK_DISPLAY_POLICY_WAIT_FOREVER; }
    if (idle.action != INK_DISPLAY_POLICY_ACTION_NONE) {
        run_cleanup(idle.action);
        return 0;
    }
    const uint32_t power_wait = power_off_wait_ms(now);
    if (power_wait == 0) {
        ink_display_report_t report = {.take_us = esp_timer_get_time()};
        power_off(&report);
        notify_drawn(&report);
        return idle.wait_ms;
    }
    return idle.wait_ms < power_wait ? idle.wait_ms : power_wait;
}

static TickType_t ticks_for(uint32_t wait_ms) {
    return wait_ms == INK_DISPLAY_POLICY_WAIT_FOREVER ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms) + 1;
}

/* New frames always win: at most one cleanup per loop, then check notifications again. */
static void task_main(void *user_data) {
    (void)user_data;
    uint32_t wait_ms = INK_DISPLAY_POLICY_WAIT_FOREVER;
    for (;;) {
        uint32_t   bits     = 0;
        const bool notified = xTaskNotifyWait(0, UINT32_MAX, &bits, ticks_for(wait_ms)) == pdTRUE;
        if (bits & NOTIFY_OPTIONS) { apply_requested_options(); }
        if (bits & NOTIFY_FRAME) { draw_latest(); }
        wait_ms = notified && (bits & NOTIFY_FRAME) ? 0 : run_idle();
    }
}

/* ---- Start ---- */

static esp_err_t check_config(const ink_display_config_t *config) {
    ESP_RETURN_ON_FALSE(config != nullptr && config->device != nullptr && config->device->operations != nullptr, ESP_ERR_INVALID_ARG, TAG,
                        "null device");
    const ink_hal_display_operations_t   *operations   = config->device->operations;
    const ink_hal_display_capabilities_t *capabilities = &config->device->capabilities;
    ESP_RETURN_ON_FALSE(operations->draw && operations->power_off && operations->read_temperature, ESP_ERR_INVALID_ARG, TAG,
                        "null operation");
    ESP_RETURN_ON_FALSE(capabilities->profile_count > 0 && capabilities->profiles && capabilities->purpose_defaults, ESP_ERR_INVALID_ARG,
                        TAG, "no profile");
    const bool options_ok = config->options == nullptr || options_valid(config->options, capabilities);
    ESP_RETURN_ON_FALSE(options_ok, ESP_ERR_INVALID_ARG, TAG, "options");
    return ESP_OK;
}

static void free_buffers(void) {
    for (int i = 0; i < INK_FRAME_SLOTS_COUNT; i++) {
        heap_caps_free(s_frames[i].buffer);
        s_frames[i].buffer = nullptr;
    }
    heap_caps_free(s_old_plane);
    heap_caps_free(s_pixels);
    s_old_plane = nullptr;
    s_pixels    = nullptr;
}

/* WARN: the 3 frames and the old plane stay in internal RAM (the driver sends them by SPI DMA); pixel_state in PSRAM (CPU only). */
static esp_err_t allocate_buffers(size_t pixel_count) {
    for (int i = 0; i < INK_FRAME_SLOTS_COUNT; i++) {
        s_frames[i] = (frame_t){.buffer = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)};
    }
    s_old_plane = heap_caps_malloc(s_frame_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_pixels    = heap_caps_malloc(pixel_count, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_pixels == nullptr) {
        ESP_LOGW(TAG, "no PSRAM: pixel_state goes to internal RAM");
        s_pixels = heap_caps_malloc(pixel_count, MALLOC_CAP_8BIT);
    }

    bool allocated = s_old_plane != nullptr && s_pixels != nullptr;
    for (int i = 0; i < INK_FRAME_SLOTS_COUNT; i++) { allocated = allocated && s_frames[i].buffer != nullptr; }
    if (!allocated) {
        free_buffers();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/* The background frame drawn before sleep becomes frame 0, the shown frame. */
static void restore_background(const uint8_t *background) {
    memcpy(s_frames[0].buffer, background, s_frame_bytes);
    if (ink_frame_slots_assume_shown(&s_slots, 0) != INK_FRAME_SLOTS_OK) { ESP_LOGE(TAG, "frame_slots assume_shown"); }
    ink_display_policy_assume_shown(&s_policy);
}

esp_err_t ink_display_start(const ink_display_config_t *config) {
    if (s_task != nullptr) { return ESP_ERR_INVALID_STATE; }
    ESP_RETURN_ON_ERROR(check_config(config), TAG, "config");
    const ink_hal_display_capabilities_t *capabilities = &config->device->capabilities;
    s_device                                           = *config->device;
    s_on_drawn                                         = config->on_report;
    s_user_data                                        = config->user_data;
    s_options     = config->options != nullptr ? *config->options : ink_display_default_options(capabilities);
    s_frame_bytes = ((size_t)capabilities->width + 7) / 8 * capabilities->height;
    if (ink_frame_slots_init(&s_slots) != INK_FRAME_SLOTS_OK) { return ESP_ERR_INVALID_STATE; }
    ESP_RETURN_ON_ERROR(allocate_buffers((size_t)capabilities->width * capabilities->height), TAG, "buffers");

    const ink_display_policy_config_t policy = policy_config(capabilities, &s_options);
    if (ink_display_policy_init(&s_policy, &policy, s_pixels) != INK_DISPLAY_POLICY_OK) {
        free_buffers();
        return ESP_ERR_INVALID_ARG;
    }
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
