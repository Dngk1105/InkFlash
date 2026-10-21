#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "drv_led.h"
#include "ink_hal_led.h"

/* WS2812: một bit = một xung cao rồi thấp. Độ phân giải RMT 10 MHz → 1 tick = 100 ns. */
#define RMT_RES_HZ 10000000
#define T0H 3 /* 0,3 µs */
#define T0L 9 /* 0,9 µs */
#define T1H 9
#define T1L 3

static rmt_channel_handle_t s_chan;
static rmt_encoder_handle_t s_encoder;
static uint8_t s_brightness = 255;

esp_err_t led_ws2812_init(const drv_led_cfg_t *cfg)
{
    if (s_chan) {
        return ESP_OK;
    }
    s_brightness = cfg->brightness ? cfg->brightness : 255;
    const rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = cfg->pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RES_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 2,
    };
    esp_err_t err = rmt_new_tx_channel(&tx_cfg, &s_chan);
    if (err != ESP_OK) {
        return err;
    }
    const rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = { .level0 = 1, .duration0 = T0H, .level1 = 0, .duration1 = T0L },
        .bit1 = { .level0 = 1, .duration0 = T1H, .level1 = 0, .duration1 = T1L },
        .flags.msb_first = 1,
    };
    err = rmt_new_bytes_encoder(&enc_cfg, &s_encoder);
    if (err != ESP_OK) {
        return err;
    }
    return rmt_enable(s_chan);
}

static esp_err_t ws_set_rgb(void *ctx, uint8_t r, uint8_t g, uint8_t b)
{
    /* WS2812 nhận thứ tự G, R, B. Giới hạn độ sáng theo trần của board. */
    const uint8_t grb[3] = {
        (uint8_t)(g * s_brightness / 255),
        (uint8_t)(r * s_brightness / 255),
        (uint8_t)(b * s_brightness / 255),
    };
    const rmt_transmit_config_t tx = { .loop_count = 0 };
    esp_err_t err = rmt_transmit(s_chan, s_encoder, grb, sizeof(grb), &tx);
    if (err != ESP_OK) {
        return err;
    }
    return rmt_tx_wait_all_done(s_chan, 50);
}

const ink_led_ops_t led_ws2812_ops = { .set_rgb = ws_set_rgb };
