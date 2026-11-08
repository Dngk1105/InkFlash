/* Design: docs/design/hardware-variants.md */
#pragma once

/* Pin map and drivers of the board. Upper layers get devices through the ink_hal interfaces only. */
#include "esp_err.h"
#include "ink_hal.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Init every driver from the pin map.
 * @par Called from
 * app_main, once, before any other ink_bsp function.
 * @retval ESP_ERR_INVALID_STATE already initialized.
 * @return Other codes come from the drivers, spi_bus_initialize() or gpio_install_isr_service().
 */
esp_err_t ink_bsp_init(void);

/**
 * @brief Get the button devices of the board; index 0 = A, 1 = B.
 * @par Called from
 * app_main, after ink_bsp_init().
 * @retval ESP_ERR_INVALID_ARG a pointer is NULL.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 */
esp_err_t ink_bsp_buttons(const ink_hal_button_device_t **out_devices, size_t *out_count);

/**
 * @brief Get the display device of the board.
 * @par Called from
 * app_main, after ink_bsp_init().
 * @retval ESP_ERR_INVALID_ARG out_device is NULL.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 */
esp_err_t ink_bsp_display(const ink_hal_display_device_t **out_device);
