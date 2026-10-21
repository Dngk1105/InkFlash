#include "hw_i2s.h"

#define SAMPLE_RATE_HZ 16000

static struct {
    gpio_num_t bclk, ws, din, dout;
    bool want_rx, want_tx;
} s_req = { GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, false, false };

static i2s_chan_handle_t s_rx, s_tx;
static bool s_created;

void hw_i2s_request_rx(gpio_num_t bclk, gpio_num_t ws, gpio_num_t din)
{
    s_req.bclk = bclk;
    s_req.ws = ws;
    s_req.din = din;
    s_req.want_rx = true;
}

void hw_i2s_request_tx(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout)
{
    s_req.bclk = bclk;
    s_req.ws = ws;
    s_req.dout = dout;
    s_req.want_tx = true;
}

static i2s_std_config_t std_config(gpio_num_t din, gpio_num_t dout)
{
    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = s_req.bclk,
            .ws = s_req.ws,
            .dout = dout,
            .din = din,
        },
    };
    return cfg;
}

static esp_err_t create_channels(void)
{
    if (s_created) {
        return ESP_OK;
    }
    if (!s_req.want_rx && !s_req.want_tx) {
        return ESP_ERR_INVALID_STATE;
    }
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear_after_cb = true; /* hết dữ liệu thì phát im lặng, không lặp lại bộ đệm cuối (tránh tiếng bíp kéo dài) */
    esp_err_t err = i2s_new_channel(&chan_cfg, s_req.want_tx ? &s_tx : NULL, s_req.want_rx ? &s_rx : NULL);
    if (err != ESP_OK) {
        return err;
    }
    if (s_tx) {
        i2s_std_config_t cfg = std_config(I2S_GPIO_UNUSED, s_req.dout);
        err = i2s_channel_init_std_mode(s_tx, &cfg);
        if (err != ESP_OK) {
            return err;
        }
    }
    if (s_rx) {
        i2s_std_config_t cfg = std_config(s_req.din, I2S_GPIO_UNUSED);
        err = i2s_channel_init_std_mode(s_rx, &cfg);
        if (err != ESP_OK) {
            return err;
        }
    }
    s_created = true;
    return ESP_OK;
}

esp_err_t hw_i2s_get_rx(i2s_chan_handle_t *out)
{
    esp_err_t err = create_channels();
    if (err != ESP_OK) {
        return err;
    }
    if (!s_rx) {
        return ESP_ERR_NOT_FOUND;
    }
    *out = s_rx;
    return ESP_OK;
}

esp_err_t hw_i2s_get_tx(i2s_chan_handle_t *out)
{
    esp_err_t err = create_channels();
    if (err != ESP_OK) {
        return err;
    }
    if (!s_tx) {
        return ESP_ERR_NOT_FOUND;
    }
    *out = s_tx;
    return ESP_OK;
}
