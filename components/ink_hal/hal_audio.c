#include "ink_hal_audio.h"
#include "ink_health.h"

static const ink_audio_out_ops_t *s_out;
static void *s_out_ctx;
static const ink_audio_in_ops_t *s_in;
static void *s_in_ctx;

esp_err_t ink_audio_out_attach(const ink_audio_out_ops_t *ops, void *ctx)
{
    if (!ops || !ops->start || !ops->write || !ops->stop) {
        return ESP_ERR_INVALID_ARG;
    }
    s_out_ctx = ctx;
    s_out = ops;
    return ESP_OK;
}

esp_err_t ink_audio_in_attach(const ink_audio_in_ops_t *ops, void *ctx)
{
    if (!ops || !ops->start || !ops->read || !ops->stop) {
        return ESP_ERR_INVALID_ARG;
    }
    s_in_ctx = ctx;
    s_in = ops;
    return ESP_OK;
}

bool ink_audio_out_present(void) { return s_out != NULL; }
bool ink_audio_in_present(void) { return s_in != NULL; }

esp_err_t ink_audio_out_start(void)
{
    return s_out ? s_out->start(s_out_ctx, INK_AUDIO_RATE_HZ) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_audio_out_write(const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms)
{
    if (!s_out) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = s_out->write(s_out_ctx, buf, n, done, timeout_ms);
    ink_health_report(INK_COMP_AUDIO_OUT, err == ESP_OK || err == ESP_ERR_TIMEOUT);
    return err;
}

esp_err_t ink_audio_out_stop(void)
{
    return s_out ? s_out->stop(s_out_ctx) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_audio_in_start(void)
{
    return s_in ? s_in->start(s_in_ctx, INK_AUDIO_RATE_HZ) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_audio_in_read(int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms)
{
    if (!s_in) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = s_in->read(s_in_ctx, buf, n, got, timeout_ms);
    ink_health_report(INK_COMP_AUDIO_IN, err == ESP_OK || err == ESP_ERR_TIMEOUT);
    return err;
}

esp_err_t ink_audio_in_stop(void)
{
    return s_in ? s_in->stop(s_in_ctx) : ESP_ERR_NOT_FOUND;
}
