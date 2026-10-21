#include "driver/i2s_std.h"
#include "hw_i2s.h"
#include "ink_hal_audio.h"

/*
 * INMP441 (mic MEMS I2S). Dữ liệu 24 bit nằm ở bit cao của khung 32 bit, kênh trái khi chân L/R nối GND.
 * Chân L/R nối 3V3 thì dữ liệu ở kênh phải: khi đó đổi chỉ số lấy mẫu bên dưới.
 */
#define FRAMES_PER_READ 64

static i2s_chan_handle_t s_rx;
static bool s_running;

static esp_err_t mic_start(void *ctx, uint32_t rate_hz)
{
    if (rate_hz != INK_AUDIO_RATE_HZ) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = hw_i2s_get_rx(&s_rx);
    if (err != ESP_OK) {
        return err;
    }
    if (!s_running) {
        err = i2s_channel_enable(s_rx);
        if (err != ESP_OK) {
            return err;
        }
        s_running = true;
    }
    return ESP_OK;
}

static esp_err_t mic_read(void *ctx, int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms)
{
    if (!s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    int32_t raw[FRAMES_PER_READ * 2]; /* stereo: trái, phải */
    size_t total = 0;
    while (total < n) {
        size_t want = n - total < FRAMES_PER_READ ? n - total : FRAMES_PER_READ;
        size_t bytes = 0;
        esp_err_t err = i2s_channel_read(s_rx, raw, want * 2 * sizeof(int32_t), &bytes, timeout_ms);
        size_t frames = bytes / (2 * sizeof(int32_t));
        for (size_t i = 0; i < frames; i++) {
            buf[total + i] = (int16_t)(raw[2 * i] >> 16);
        }
        total += frames;
        if (err != ESP_OK) {
            *got = total;
            return total ? ESP_OK : err;
        }
    }
    *got = total;
    return ESP_OK;
}

static esp_err_t mic_stop(void *ctx)
{
    if (s_running) {
        s_running = false;
        return i2s_channel_disable(s_rx);
    }
    return ESP_OK;
}

const ink_mic_ops_t mic_inmp441_ops = { .start = mic_start, .read = mic_read, .stop = mic_stop };
