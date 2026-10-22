#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_hal_audio.h"

static uint32_t s_rate;
static uint8_t s_volume = 50;

static esp_err_t fake_start(void *ctx, uint32_t rate_hz)
{
    s_rate = rate_hz;
    return ESP_OK;
}

/* Bỏ dữ liệu, chờ đúng thời lượng phát thật. */
static esp_err_t fake_write(void *ctx, const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms)
{
    if (!s_rate) {
        return ESP_ERR_INVALID_STATE;
    }
    vTaskDelay(pdMS_TO_TICKS(n * 1000 / s_rate));
    *done = n;
    return ESP_OK;
}

static esp_err_t fake_stop(void *ctx)
{
    s_rate = 0;
    return ESP_OK;
}

static esp_err_t fake_set_volume(void *ctx, uint8_t percent)
{
    s_volume = percent;
    return ESP_OK;
}

const ink_speaker_ops_t speaker_fake_ops = {
    .start = fake_start,
    .write = fake_write,
    .stop = fake_stop,
    .set_volume = fake_set_volume,
};
