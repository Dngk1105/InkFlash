#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "drv_audio.h"
#include "hw_i2s.h"
#include "ink_hal_audio.h"

/*
 * MAX98357A (amp I2S, lớp D). Nhận khung 32 bit stereo; chân SD_MODE nối chân `sd` để bật/tắt amp
 * (chưa đấu thì amp luôn bật). Âm lượng mặc định 25% như demo (biên độ 0.25).
 */
#define FRAMES_PER_WRITE 128
#define DEFAULT_VOLUME_PCT 25

static i2s_chan_handle_t s_tx;
static bool s_running;
static uint8_t s_volume = DEFAULT_VOLUME_PCT;

static void amp_enable(const drv_speaker_cfg_t *cfg, bool on)
{
    if (cfg->sd != GPIO_NUM_NC) {
        gpio_set_level(cfg->sd, on ? 1 : 0);
    }
}

static esp_err_t spk_start(void *ctx, uint32_t rate_hz)
{
    const drv_speaker_cfg_t *cfg = ctx;
    if (rate_hz != INK_AUDIO_RATE_HZ) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = hw_i2s_get_tx(&s_tx);
    if (err != ESP_OK) {
        return err;
    }
    if (cfg->sd != GPIO_NUM_NC) {
        const gpio_config_t sd = { .pin_bit_mask = 1ULL << cfg->sd, .mode = GPIO_MODE_OUTPUT };
        gpio_config(&sd);
    }
    if (!s_running) {
        err = i2s_channel_enable(s_tx);
        if (err != ESP_OK) {
            return err;
        }
        s_running = true;
    }
    amp_enable(cfg, true);
    return ESP_OK;
}

static esp_err_t spk_write(void *ctx, const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms)
{
    if (!s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    int32_t out[FRAMES_PER_WRITE * 2];
    size_t total = 0;
    while (total < n) {
        size_t chunk = n - total < FRAMES_PER_WRITE ? n - total : FRAMES_PER_WRITE;
        for (size_t i = 0; i < chunk; i++) {
            int32_t s = (int32_t)buf[total + i] * s_volume / 100;
            int32_t frame = (int32_t)((uint32_t)s << 16); /* 16 bit → bit cao của khung 32 bit */
            out[2 * i] = frame;
            out[2 * i + 1] = frame; /* mono → cả hai kênh */
        }
        size_t bytes = 0;
        esp_err_t err = i2s_channel_write(s_tx, out, chunk * 2 * sizeof(int32_t), &bytes, timeout_ms);
        total += bytes / (2 * sizeof(int32_t));
        if (err != ESP_OK) {
            *done = total;
            return total ? ESP_OK : err;
        }
    }
    *done = total;
    return ESP_OK;
}

static esp_err_t spk_stop(void *ctx)
{
    amp_enable(ctx, false);
    if (s_running) {
        s_running = false;
        return i2s_channel_disable(s_tx);
    }
    return ESP_OK;
}

static esp_err_t spk_set_volume(void *ctx, uint8_t percent)
{
    s_volume = percent;
    return ESP_OK;
}

const ink_speaker_ops_t speaker_max98357a_ops = {
    .start = spk_start,
    .write = spk_write,
    .stop = spk_stop,
    .set_volume = spk_set_volume,
};
