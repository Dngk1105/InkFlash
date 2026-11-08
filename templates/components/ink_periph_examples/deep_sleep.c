/* Design: docs/design/esp-idf-api.md */

/*
 * Based on: examples/system/deep_sleep.
 * Uses esp_sleep_enable_ext1_wakeup_io (not esp_sleep_enable_ext1_wakeup) and
 * esp_sleep_get_wakeup_causes (not esp_sleep_get_wakeup_cause, deprecated in v6.1).
 */
#include "ink_periph_examples.h"

#include "driver/rtc_io.h"
#include "esp_attr.h"
#include "esp_bit_defs.h"
#include "esp_sleep.h"

/* RTC_DATA_ATTR variables live in RTC memory: kept across deep sleep, lost on
 * power loss. */
static RTC_DATA_ATTR uint32_t s_rtc_counter;

void ink_periph_examples_deep_sleep(uint64_t wake_pin_mask) {
    /* Button to GND: the RTC pull-up holds the pin high during sleep. The pin
     * must be an RTC GPIO. */
    for (int pin = 0; pin < GPIO_NUM_MAX; pin++) {
        if (wake_pin_mask & (1ULL << pin)) {
            rtc_gpio_pulldown_dis(pin);
            rtc_gpio_pullup_en(pin);
        }
    }
    esp_sleep_enable_ext1_wakeup_io(wake_pin_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

bool ink_periph_examples_woke_by_button(void) {
    return (esp_sleep_get_wakeup_causes() & BIT(ESP_SLEEP_WAKEUP_EXT1)) != 0;
}

uint32_t ink_periph_examples_rtc_counter_next(void) {
    return ++s_rtc_counter;
}
