/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/peripherals/gpio/generic_gpio */
#include "ink_periph_examples.h"

#include "esp_check.h"

static const char *TAG = "periph_gpio";

esp_err_t ink_periph_examples_gpio_isr_init(gpio_num_t pin, gpio_isr_t isr, void *arg)
{
    const gpio_config_t io = {
        .pin_bit_mask = 1ULL << pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config");
    /* Dịch vụ ngắt GPIO cài một lần cho cả chip; gọi lại trả ESP_ERR_INVALID_STATE, coi như đã có. */
    const esp_err_t err = gpio_install_isr_service(0);
    ESP_RETURN_ON_FALSE(err == ESP_OK || err == ESP_ERR_INVALID_STATE, err, TAG, "gpio_install_isr_service");
    return gpio_isr_handler_add(pin, isr, arg);
}
