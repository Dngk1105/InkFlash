/* Design: docs/design/tasks.md */
#include "ink_platform.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_rom_crc.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ink_platform";

/* ---- Time ---- */

uint32_t IRAM_ATTR ink_platform_now_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/* ---- One-shot timer ---- */
/* ink_platform_timer_t is an esp_timer_handle_t, cast both ways. */

esp_err_t ink_platform_timer_create(ink_platform_timer_callback_t callback, void *user_data, const char *name,
                                    ink_platform_timer_t *out_timer) {
    ESP_RETURN_ON_FALSE(callback && out_timer, ESP_ERR_INVALID_ARG, TAG, "null arg");
    const esp_timer_create_args_t timer_args = {
        .callback        = callback,
        .arg             = user_data,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = name,
    };
    esp_timer_handle_t handle = nullptr;
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &handle), TAG, "esp_timer_create");
    *out_timer = (ink_platform_timer_t)handle;
    return ESP_OK;
}

esp_err_t ink_platform_timer_start(ink_platform_timer_t timer, uint32_t delay_ms) {
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "null timer");
    ESP_RETURN_ON_ERROR(ink_platform_timer_stop(timer), TAG, "stop");
    return esp_timer_start_once((esp_timer_handle_t)timer, (uint64_t)delay_ms * 1000);
}

esp_err_t ink_platform_timer_stop(ink_platform_timer_t timer) {
    ESP_RETURN_ON_FALSE(timer, ESP_ERR_INVALID_ARG, TAG, "null timer");
    const esp_err_t error = esp_timer_stop((esp_timer_handle_t)timer);
    /* ESP_ERR_INVALID_STATE: the timer is not running */
    return error == ESP_ERR_INVALID_STATE ? ESP_OK : error;
}

/* ---- RTC slots ---- */
/* Based on: templates/components/ink_periph_examples/deep_sleep.c (RTC_DATA_ATTR, ext1 _io, reset reason). */

/* REVIEW(platform): sizes picked by hand; header 12 B, scene snapshot 40 B, the rest of APP is app-level state. */
#define RTC_SLOT_SYSTEM_BYTES 32
#define RTC_SLOT_APP_BYTES    256
#define RTC_BYTES             (RTC_SLOT_SYSTEM_BYTES + RTC_SLOT_APP_BYTES)

static const uint16_t s_rtc_slot_sizes[INK_PLATFORM_RTC_SLOT_COUNT] = {
    [INK_PLATFORM_RTC_SLOT_SYSTEM] = RTC_SLOT_SYSTEM_BYTES,
    [INK_PLATFORM_RTC_SLOT_APP]    = RTC_SLOT_APP_BYTES,
};

static_assert(RTC_BYTES <= 8192, "snapshots must fit RTC slow memory (8 KB)");

static RTC_DATA_ATTR uint32_t s_rtc_words[RTC_BYTES / 4]; /* uint32_t: every slot starts 4-byte aligned */

esp_err_t ink_platform_rtc_slot(ink_platform_rtc_slot_t slot, uint8_t **out_bytes, size_t *out_size) {
    ESP_RETURN_ON_FALSE(slot < INK_PLATFORM_RTC_SLOT_COUNT && out_bytes && out_size, ESP_ERR_INVALID_ARG, TAG, "bad arg");
    size_t offset = 0;
    for (uint8_t i = 0; i < slot; i++) { offset += s_rtc_slot_sizes[i]; }
    *out_bytes = (uint8_t *)s_rtc_words + offset;
    *out_size  = s_rtc_slot_sizes[slot];
    return ESP_OK;
}

uint32_t ink_platform_crc32(const uint8_t *data, size_t length) {
    return esp_rom_crc32_le(0, data, (uint32_t)length);
}

/* ---- Reset reason and deep sleep ---- */

bool ink_platform_woke_from_deep_sleep(void) {
    return esp_reset_reason() == ESP_RST_DEEPSLEEP;
}

/* true while any pin of the mask reads low (a button to GND still pressed). */
static bool any_pin_low(uint64_t pin_mask) {
    for (int pin = 0; pin < GPIO_NUM_MAX; pin++) {
        if ((pin_mask & (1ULL << pin)) && gpio_get_level(pin) == 0) { return true; }
    }
    return false;
}

void ink_platform_deep_sleep(uint64_t wake_pin_mask) {
    /* ext1 wakes on ANY_LOW: sleeping while a button is still held (HOLD_AB_LONG fires before release) wakes at once.
     * WARN: a stuck button keeps the machine awake here. */
    if (any_pin_low(wake_pin_mask)) { ESP_LOGI(TAG, "deep sleep: waiting for the wake buttons to be released"); }
    while (any_pin_low(wake_pin_mask)) { vTaskDelay(pdMS_TO_TICKS(20)); }
    /* Button to GND: the RTC pull-up holds the pin high during sleep. */
    for (int pin = 0; pin < GPIO_NUM_MAX; pin++) {
        if (wake_pin_mask & (1ULL << pin)) {
            rtc_gpio_pulldown_dis(pin);
            rtc_gpio_pullup_en(pin);
        }
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_ext1_wakeup_io(wake_pin_mask, ESP_EXT1_WAKEUP_ANY_LOW));
    esp_deep_sleep_start();
}
