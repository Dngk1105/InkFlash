/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/peripherals/i2s/i2s_basic/i2s_std. Không dùng driver/i2s.h (đã xoá trong v6.1). */
#include "ink_periph_examples.h"

#include "esp_check.h"

static const char *TAG = "periph_i2s";

esp_err_t ink_periph_examples_i2s_tx_init(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, uint32_t sample_rate,
                                          i2s_chan_handle_t *out_tx_channel)
{
    const i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel_config, out_tx_channel, nullptr), TAG, "i2s_new_channel");
    i2s_std_config_t std_config = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    };
    std_config.gpio_cfg = (i2s_std_gpio_config_t){
        .mclk         = I2S_GPIO_UNUSED,
        .bclk         = bclk,
        .ws           = ws,
        .dout         = dout,
        .din          = I2S_GPIO_UNUSED,
        .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(*out_tx_channel, &std_config), TAG, "i2s_channel_init_std_mode");
    return i2s_channel_enable(*out_tx_channel);
}
