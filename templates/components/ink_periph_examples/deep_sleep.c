/* Thiết kế: docs/design/esp-idf-api.md */

/*
 * Dựa theo: examples/system/deep_sleep.
 * Dùng esp_sleep_enable_ext1_wakeup_io (không dùng
 * esp_sleep_enable_ext1_wakeup) và esp_sleep_get_wakeup_causes (không dùng
 * esp_sleep_get_wakeup_cause, đã deprecated trong v6.1).
 */
#include "ink_periph_examples.h"

#include "driver/rtc_io.h"
#include "esp_attr.h"
#include "esp_bit_defs.h"
#include "esp_sleep.h"

/* Biến có RTC_DATA_ATTR nằm trong RTC memory: còn giữ qua ngủ sâu, mất khi mất
 * điện. */
static RTC_DATA_ATTR uint32_t s_rtc_counter;

void ink_periph_examples_deep_sleep(uint64_t wake_pin_mask) {
    /* Nút nối GND: giữ chân ở mức cao bằng pull-up của RTC trong lúc ngủ. Chân
     * phải là RTC GPIO. */
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
