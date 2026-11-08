/* Design: docs/design/hardware-variants.md */
#pragma once

/*
 * Template of a driver: one GPIO input pin, implementing ink_hal_example.
 * The driver does not know its pin: the BSP passes it in config at init.
 * The caller provides the driver memory (struct ink_drv_example_t); the driver never mallocs.
 */
#include "driver/gpio.h"
#include "esp_err.h"
#include "ink_hal_example.h"
#include <stdbool.h>

typedef struct {
    gpio_num_t pin;
    bool       active_low; /**< true: level 0 means active (e.g. a button to GND) */
} ink_drv_example_config_t;

typedef struct {
    ink_drv_example_config_t config;
} ink_drv_example_t;

/**
 * @brief Configure the pin and fill `out_device` for upper layers to use through the HAL.
 * @par Called from
 * BSP, once at boot.
 * @retval ESP_ERR_INVALID_ARG a pointer is NULL.
 * @retval other gpio_config() error.
 */
esp_err_t ink_drv_example_init(ink_drv_example_t *driver, const ink_drv_example_config_t *config, ink_hal_example_device_t *out_device);
