#include "hw_i2c.h"

static i2c_master_bus_handle_t s_bus;
static gpio_num_t s_sda = GPIO_NUM_NC, s_scl = GPIO_NUM_NC;

esp_err_t hw_i2c_get_bus(gpio_num_t sda, gpio_num_t scl, i2c_master_bus_handle_t *out)
{
    if (sda == GPIO_NUM_NC || scl == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_bus) {
        if (sda != s_sda || scl != s_scl) {
            return ESP_ERR_INVALID_STATE; /* một bus I2C, hai cặp chân khác nhau */
        }
        *out = s_bus;
        return ESP_OK;
    }
    const i2c_master_bus_config_t cfg = {
        .i2c_port = -1,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, /* breadboard: thêm điện trở kéo lên ngoài nếu bus lỗi */
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &s_bus);
    if (err != ESP_OK) {
        return err;
    }
    s_sda = sda;
    s_scl = scl;
    *out = s_bus;
    return ESP_OK;
}
