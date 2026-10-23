#include "ink_hal_audio.h"
#include "ink_health.h"

static const ink_mic_ops_t *s_mic;
static void *s_mic_ctx;
static const ink_speaker_ops_t *s_spk;
static void *s_spk_ctx;

esp_err_t ink_mic_attach(const ink_mic_ops_t *ops, void *ctx)
{
    if (!ops || !ops->start || !ops->read || !ops->stop) {
        return ESP_ERR_INVALID_ARG;
    }
    s_mic_ctx = ctx;
    s_mic = ops;
    return ESP_OK;
}

bool ink_mic_present(void) { return s_mic != NULL; }

esp_err_t ink_mic_start(void)
{
    return s_mic ? s_mic->start(s_mic_ctx, INK_AUDIO_RATE_HZ) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_mic_read(int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms)
{
    if (!s_mic) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = s_mic->read(s_mic_ctx, buf, n, got, timeout_ms);
    ink_health_report(INK_COMP_AUDIO_HW, err == ESP_OK || err == ESP_ERR_TIMEOUT);
    return err;
}

esp_err_t ink_mic_stop(void)
{
    return s_mic ? s_mic->stop(s_mic_ctx) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_speaker_attach(const ink_speaker_ops_t *ops, void *ctx)
{
    if (!ops || !ops->start || !ops->write || !ops->stop) {
        return ESP_ERR_INVALID_ARG;
    }
    s_spk_ctx = ctx;
    s_spk = ops;
    return ESP_OK;
}

bool ink_speaker_present(void) { return s_spk != NULL; }

esp_err_t ink_speaker_start(void)
{
    return s_spk ? s_spk->start(s_spk_ctx, INK_AUDIO_RATE_HZ) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_speaker_write(const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms)
{
    if (!s_spk) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_err_t err = s_spk->write(s_spk_ctx, buf, n, done, timeout_ms);
    ink_health_report(INK_COMP_AUDIO_HW, err == ESP_OK || err == ESP_ERR_TIMEOUT);
    return err;
}

esp_err_t ink_speaker_stop(void)
{
    return s_spk ? s_spk->stop(s_spk_ctx) : ESP_ERR_NOT_FOUND;
}

esp_err_t ink_speaker_set_volume(uint8_t percent)
{
    if (!s_spk) {
        return ESP_ERR_NOT_FOUND;
    }
    if (percent > 100) {
        percent = 100;
    }
    return s_spk->set_volume ? s_spk->set_volume(s_spk_ctx, percent) : ESP_ERR_NOT_SUPPORTED;
}
