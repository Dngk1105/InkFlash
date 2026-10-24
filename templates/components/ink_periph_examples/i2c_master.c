/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/peripherals/i2c/i2c_basic. Không dùng driver/i2c.h (driver cũ). */
#include "ink_periph_examples.h"

#include "esp_check.h"

#define XFER_TIMEOUT_MS 50

static const char *TAG = "periph_i2c";

esp_err_t ink_periph_examples_i2c_init(gpio_num_t sda, gpio_num_t scl, uint16_t addr, uint32_t hz,
                                       i2c_master_bus_handle_t *out_bus, i2c_master_dev_handle_t *out_dev)
{
    const i2c_master_bus_config_t bus = {
        .i2c_port = -1, /* tự chọn cổng */
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus, out_bus), TAG, "i2c_new_master_bus");
    const i2c_device_config_t dev = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = hz,
    };
    return i2c_master_bus_add_device(*out_bus, &dev, out_dev);
}

esp_err_t ink_periph_examples_i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *out, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, out, len, XFER_TIMEOUT_MS);
}
