/* Design: docs/design/esp-idf-api.md */
#pragma once

/*
 * Templates of correct ESP-IDF v6.1 API use, one .c file per peripheral, each naming the official
 * example it follows. Pins are always parameters (the BSP picks pins), never hardcoded.
 * NOTE: these functions exist only to be built and copied; the firmware does not call them,
 * so "Called from" below names where copied code would run.
 */
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ---- GPIO ---- */

/**
 * @brief GPIO input with an interrupt on both edges.
 * @par Called from
 * A task, once at init.
 * @param isr_handler Runs in the ISR; must be IRAM_ATTR.
 * @retval other gpio_config(), gpio_install_isr_service() or gpio_isr_handler_add() error.
 */
esp_err_t ink_periph_examples_gpio_isr_init(gpio_num_t pin, gpio_isr_t isr_handler, void *user_data);

/* ---- SPI ---- */

/**
 * @brief Write-only SPI master (like the e-paper panel): init the bus and add one device.
 * @par Called from
 * A task, once at init.
 * @retval other spi_bus_initialize() or spi_bus_add_device() error.
 */
esp_err_t ink_periph_examples_spi_init(spi_host_device_t host, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t cs, int clock_hz,
                                       spi_device_handle_t *out_device);

/**
 * @brief Send one block of data and wait until done.
 * @par Called from
 * The task that owns the device.
 * @retval other spi_device_polling_transmit() error.
 */
esp_err_t ink_periph_examples_spi_write(spi_device_handle_t device, const uint8_t *data, size_t length);

/* ---- I2C ---- */

/**
 * @brief I2C master (new driver, not driver/i2c.h): create the bus and add one 7-bit device.
 * @par Called from
 * A task, once at init.
 * @retval other i2c_new_master_bus() or i2c_master_bus_add_device() error.
 */
esp_err_t ink_periph_examples_i2c_init(gpio_num_t sda, gpio_num_t scl, uint16_t address, uint32_t clock_hz,
                                       i2c_master_bus_handle_t *out_bus, i2c_master_dev_handle_t *out_device);

/**
 * @brief Write the register address, then read `length` bytes.
 * @par Called from
 * The task that owns the device.
 * @retval other i2c_master_transmit_receive() error.
 */
esp_err_t ink_periph_examples_i2c_read_register(i2c_master_dev_handle_t device, uint8_t register_address, uint8_t *out_data, size_t length);

/* ---- I2S ---- */

/**
 * @brief Standard Philips I2S, TX channel for a speaker.
 * @par Called from
 * A task, once at init.
 * @retval other i2s_new_channel() or i2s_channel_init_std_mode() error.
 */
esp_err_t ink_periph_examples_i2s_tx_init(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, uint32_t sample_rate,
                                          i2s_chan_handle_t *out_tx_channel);

/* ---- ADC ---- */

/**
 * @brief Calibrated oneshot ADC: read the voltage of one ADC1 channel, mV.
 * @par Called from
 * Any task.
 * @retval other adc_oneshot or adc_cali error.
 */
esp_err_t ink_periph_examples_adc_read_mv(adc_channel_t channel, int *out_mv);

/* ---- Deep sleep, RTC memory ---- */

/**
 * @brief Configure the wake button (active low), then enter deep sleep. Does not return.
 * @par Called from
 * Main task, last step before sleep.
 */
void ink_periph_examples_deep_sleep(uint64_t wake_pin_mask);

/**
 * @brief Whether this boot is a wake from deep sleep by the button (EXT1).
 * @par Called from
 * Main task, at boot.
 * @retval true woken by EXT1.
 * @retval false any other reset or wake cause.
 */
bool ink_periph_examples_woke_by_button(void);

/**
 * @brief Increment a counter kept in RTC memory; it survives deep sleep.
 * @par Called from
 * Main task.
 * @return New counter value.
 */
uint32_t ink_periph_examples_rtc_counter_next(void);

/* ---- NVS ---- */

/**
 * @brief Read a u32 by key; `default_value` if the namespace or key does not exist.
 * @par Called from
 * Any task, after nvs_flash_init().
 * @retval other nvs_open() or nvs_get_u32() error.
 */
esp_err_t ink_periph_examples_nvs_get_u32(const char *namespace_name, const char *key, uint32_t default_value, uint32_t *out_value);

/**
 * @brief Write a u32 by key, then commit.
 * @par Called from
 * Any task, after nvs_flash_init().
 * @retval other nvs_open(), nvs_set_u32() or nvs_commit() error.
 */
esp_err_t ink_periph_examples_nvs_set_u32(const char *namespace_name, const char *key, uint32_t value);

/* ---- USB-Serial-JTAG ---- */

/**
 * @brief Install the USB-Serial-JTAG driver for binary I/O on the chip's USB port (a COM port on the PC).
 * @par Called from
 * A task, once at init.
 * @retval other usb_serial_jtag_driver_install() error.
 */
esp_err_t ink_periph_examples_usb_serial_jtag_init(uint32_t rx_buffer_size, uint32_t tx_buffer_size);

/**
 * @brief Read up to `length` bytes, waiting at most timeout_ms.
 * @par Called from
 * The task that owns the port.
 * @return Number of bytes read.
 */
size_t ink_periph_examples_usb_serial_jtag_read(uint8_t *buffer, size_t length, uint32_t timeout_ms);

/**
 * @brief Write all `length` bytes.
 * @par Called from
 * The task that owns the port.
 * @retval ESP_ERR_TIMEOUT the TX ring buffer did not take everything within timeout_ms.
 */
esp_err_t ink_periph_examples_usb_serial_jtag_write(const uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Route ESP-IDF logs to a private vprintf (esp_log_set_vprintf) so logs do not mix with binary data.
 * @par Called from
 * A task, once at init.
 */
void ink_periph_examples_usb_serial_jtag_take_log(void);
