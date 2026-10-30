/* Thiết kế: docs/design/input.md */
#include "ink_drv_gpio_button.h"

#include "esp_attr.h"
#include "esp_check.h"

static const char *TAG = "ink_drv_gpio_button";

/* ---- Chân GPIO ---- */

static esp_err_t configure_pin(const ink_drv_gpio_button_config_t *config, gpio_int_type_t interrupt_type) {
    const gpio_config_t pin_config = {
        .pin_bit_mask = 1ULL << config->pin,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = config->pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = interrupt_type,
    };
    return gpio_config(&pin_config);
}

static bool IRAM_ATTR is_pressed(const ink_drv_gpio_button_t *driver) {
    const bool level_high = gpio_get_level(driver->config.pin) != 0;
    return driver->config.active_low ? !level_high : level_high;
}

/* ---- Ngắt ---- */

static void IRAM_ATTR on_pin_edge(void *user_data) {
    const ink_drv_gpio_button_t *driver = user_data;
    driver->callback(driver->callback_user_data, is_pressed(driver));
}

/* ---- Bảng hàm ink_hal_button ---- */

static esp_err_t read_pressed(void *context, bool *out_pressed) {
    *out_pressed = is_pressed(context);
    return ESP_OK;
}

static esp_err_t set_callback(void *context, ink_hal_button_callback_t callback, void *user_data) {
    ink_drv_gpio_button_t *driver = context;
    ESP_RETURN_ON_FALSE(callback, ESP_ERR_INVALID_ARG, TAG, "null callback");
    ESP_RETURN_ON_FALSE(driver->callback == nullptr, ESP_ERR_INVALID_STATE, TAG, "callback already set");
    driver->callback           = callback;
    driver->callback_user_data = user_data;
    ESP_RETURN_ON_ERROR(configure_pin(&driver->config, GPIO_INTR_ANYEDGE), TAG, "gpio_config pin %d", driver->config.pin);
    return gpio_isr_handler_add(driver->config.pin, on_pin_edge, driver);
}

static const ink_hal_button_operations_t s_operations = {
    .read         = read_pressed,
    .set_callback = set_callback,
};

/* ---- Khởi tạo ---- */

esp_err_t ink_drv_gpio_button_init(ink_drv_gpio_button_t *driver, const ink_drv_gpio_button_config_t *config,
                                   ink_hal_button_device_t *out_device) {
    ESP_RETURN_ON_FALSE(driver && config && out_device, ESP_ERR_INVALID_ARG, TAG, "null arg");
    ESP_RETURN_ON_ERROR(configure_pin(config, GPIO_INTR_DISABLE), TAG, "gpio_config pin %d", config->pin);
    *driver = (ink_drv_gpio_button_t){.config = *config};

    *out_device = (ink_hal_button_device_t){
        .operations   = &s_operations,
        .context      = driver,
        .capabilities = {.has_interrupt = true},
    };
    return ESP_OK;
}
