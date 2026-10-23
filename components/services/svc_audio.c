#include "ink_service.h"
#include "svc_audio.h"
#include "svc_internal.h"

/* GĐ5: giải mã ADPCM, buffer phát cấp sẵn, bật/tắt amp theo hàng đợi. */

esp_err_t audio_play_media(const char *media_hash)
{
    (void)media_hash;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t audio_stop(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t audio_set_volume(uint8_t percent)
{
    (void)percent;
    return ESP_ERR_NOT_SUPPORTED;
}

static const ink_service_def_t s_def = {
    .name = "svc_audio",
    .comp = INK_COMP_SVC_AUDIO,
    .stack_size = 4096,
    .priority = 6,
};

esp_err_t svc_audio_start(void)
{
    return ink_service_start(&s_def, NULL);
}
