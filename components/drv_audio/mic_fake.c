#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_hal_audio.h"

static uint32_t s_rate;

static esp_err_t fake_start(void *ctx, uint32_t rate_hz)
{
    s_rate = rate_hz;
    return ESP_OK;
}

/* Trả im lặng, chờ đúng thời lượng thật để vòng lặp phía trên chạy nhịp như có mic. */
static esp_err_t fake_read(void *ctx, int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms)
{
    if (!s_rate) {
        return ESP_ERR_INVALID_STATE;
    }
    memset(buf, 0, n * sizeof(int16_t));
    vTaskDelay(pdMS_TO_TICKS(n * 1000 / s_rate));
    *got = n;
    return ESP_OK;
}

static esp_err_t fake_stop(void *ctx)
{
    s_rate = 0;
    return ESP_OK;
}

const ink_mic_ops_t mic_fake_ops = { .start = fake_start, .read = fake_read, .stop = fake_stop };
