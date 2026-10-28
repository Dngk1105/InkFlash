/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/peripherals/gpio/generic_gpio */
#include "ink_periph_examples.h"

#include "esp_check.h"

static const char *TAG = "periph_gpio";

esp_err_t ink_periph_examples_gpio_isr_init(gpio_num_t pin,
                                            gpio_isr_t isr_handler,
                                            void *user_data) {
  const gpio_config_t pin_config = {
      .pin_bit_mask = 1ULL << pin,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };
  ESP_RETURN_ON_ERROR(gpio_config(&pin_config), TAG, "gpio_config");
  /* Dịch vụ ngắt GPIO cài một lần cho cả chip; gọi lại trả
   * ESP_ERR_INVALID_STATE, coi như đã có. */
  const esp_err_t error = gpio_install_isr_service(0);
  ESP_RETURN_ON_FALSE(error == ESP_OK || error == ESP_ERR_INVALID_STATE, error,
                      TAG, "gpio_install_isr_service");
  return gpio_isr_handler_add(pin, isr_handler, user_data);
}
