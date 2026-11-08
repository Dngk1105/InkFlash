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
 * @return Other codes come from the drivers, spi_bus_initialize() or gpio_install_isr_service(). Any error is fatal:
 *         nothing is rolled back, so the call cannot be retried.
 */
esp_err_t ink_bsp_init(void);

/**
 * @brief Get the board code printed on the PCB (docs/design/hardware-variants.md), e.g. "BB0".
 * @par Called from
 * app_main, for the boot log line.
 * @return Board code; never NULL.
 */
const char *ink_bsp_board_name(void);

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

/**
 * @brief Load a `.inkw` profile blob (tools/profile_gen.py --blob) as the temporary display profile, effective from the
 *        next draw. Bench firmware only.
 * @par Called from
 * Any task.
 * @retval ESP_ERR_INVALID_ARG blob is NULL or malformed; the old temporary profile is kept.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 */
esp_err_t ink_bsp_display_load_profile(const uint8_t *blob, size_t length);

/**
 * @brief Read the OTP of the display controller into `out` (bank 0 up to the end of the waveform bands). Bench firmware only.
 * @par Called from
 * The task that calls display draw, between draws.
 * @param out_length bytes read.
 * @retval ESP_ERR_INVALID_ARG a pointer is NULL, or `capacity` is smaller than the bank.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 * @retval other SPI error.
 */
esp_err_t ink_bsp_display_read_otp(uint8_t *out, size_t capacity, size_t *out_length);

/**
 * @brief Set PSR byte 2 of the display (VCOM, VGL after a draw and at power off), effective from the next draw or power off.
 *        Bench firmware only.
 * @par Called from
 * Any task.
 * @retval ESP_ERR_INVALID_ARG `value` has bits D7-D5 set.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 */
esp_err_t ink_bsp_display_set_psr_second(uint8_t value);

/**
 * @brief Get PSR byte 2 set most recently.
 * @par Called from
 * Any task, after ink_bsp_init.
 * @return PSR byte 2.
 */
uint8_t ink_bsp_display_psr_second(void);

/**
 * @brief Replace VCOM_DC of every display profile with `value` (command 0x82 byte), effective from the next draw;
 *        0xFF = each profile's own. Bench firmware only.
 * @par Called from
 * Any task.
 * @retval ESP_ERR_INVALID_ARG `value` > 0x7E and not 0xFF.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 */
esp_err_t ink_bsp_display_set_vcom_override(uint8_t value);

/**
 * @brief Get the replacement VCOM_DC set most recently; 0xFF = each profile's own.
 * @par Called from
 * Any task, after ink_bsp_init.
 * @return VCOM_DC byte.
 */
uint8_t ink_bsp_display_vcom_override(void);

/**
 * @brief Set the frames from source off to gate off at display power off (1-4), effective from the next draw or power off.
 *        Bench firmware only.
 * @par Called from
 * Any task.
 * @retval ESP_ERR_INVALID_ARG `frames` is outside 1-4.
 * @retval ESP_ERR_INVALID_STATE not initialized.
 */
esp_err_t ink_bsp_display_set_power_off_frames(uint8_t frames);

/**
 * @brief Get the power-off frame count set most recently.
 * @par Called from
 * Any task, after ink_bsp_init.
 * @return Frames, 1-4.
 */
uint8_t ink_bsp_display_power_off_frames(void);
