/* Design: docs/design/input/input.md */
#pragma once

/*
 * Driver for a button wired straight to one GPIO pin; implements ink_hal_button and reports edges by interrupt.
 * bsp passes the pin in config. WARN: the caller allocates the driver struct in internal RAM; the ISR reads it.
 */
#include "driver/gpio.h"
#include "esp_err.h"
#include "ink_hal_button.h"
#include <stdbool.h>

typedef struct {
    gpio_num_t pin;
    bool       active_low; /**< true: level 0 = pressed (button to GND) */
    bool       pull_up;    /**< enable the internal pull-up */
} ink_drv_gpio_button_config_t;

typedef struct {
    ink_drv_gpio_button_config_t config;
    ink_hal_button_callback_t    callback;
    void                        *callback_user_data;
} ink_drv_gpio_button_t;

/**
 * @brief Configure the pin (interrupt still off) and fill `out_device`.
 *
 * The caller installs gpio_install_isr_service() before calling set_callback.
 * @par Called from
 * bsp, at boot.
 * @retval ESP_ERR_INVALID_ARG a pointer is NULL.
 * @return Other codes come from gpio_config().
 */
esp_err_t ink_drv_gpio_button_init(ink_drv_gpio_button_t *driver, const ink_drv_gpio_button_config_t *config,
                                   ink_hal_button_device_t *out_device);
