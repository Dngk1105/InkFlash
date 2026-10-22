#include "drv_audio.h"
#include "hw_i2s.h"
#include "ink_device.h"
#include "ink_hal_audio.h"
#include "sdkconfig.h"

#if CONFIG_INK_MIC_FAKE
extern const ink_mic_ops_t mic_fake_ops;
#define MIC_OPS (&mic_fake_ops)
#elif CONFIG_INK_MIC_INMP441
extern const ink_mic_ops_t mic_inmp441_ops;
#define MIC_OPS (&mic_inmp441_ops)
#endif

#if CONFIG_INK_SPEAKER_FAKE
extern const ink_speaker_ops_t speaker_fake_ops;
#define SPEAKER_OPS (&speaker_fake_ops)
#elif CONFIG_INK_SPEAKER_MAX98357A
extern const ink_speaker_ops_t speaker_max98357a_ops;
#define SPEAKER_OPS (&speaker_max98357a_ops)
#endif

#ifdef MIC_OPS
static drv_mic_cfg_t s_mic_cfg;
static esp_err_t mic_init(void *ctx) { return ink_mic_attach(MIC_OPS, &s_mic_cfg); }
static const ink_device_ops_t s_mic_dev_ops = { .init = mic_init };
#endif

#ifdef SPEAKER_OPS
static drv_speaker_cfg_t s_spk_cfg;
static esp_err_t spk_init(void *ctx) { return ink_speaker_attach(SPEAKER_OPS, &s_spk_cfg); }
static const ink_device_ops_t s_spk_dev_ops = { .init = spk_init };
#endif

esp_err_t drv_mic_register(const drv_mic_cfg_t *cfg)
{
#ifdef MIC_OPS
    s_mic_cfg = *cfg;
#if CONFIG_INK_MIC_INMP441
    if (cfg->bclk == GPIO_NUM_NC || cfg->ws == GPIO_NUM_NC || cfg->din == GPIO_NUM_NC) {
        return ESP_OK; /* board chưa đấu mic: không đăng ký */
    }
    hw_i2s_request_rx(cfg->bclk, cfg->ws, cfg->din);
#endif
    ink_device_t dev = { .name = "mic", .comp = INK_COMP_AUDIO_HW, .ops = &s_mic_dev_ops, .optional = true };
    return ink_device_register(&dev);
#else
    (void)cfg;
    return ESP_OK;
#endif
}

esp_err_t drv_speaker_register(const drv_speaker_cfg_t *cfg)
{
#ifdef SPEAKER_OPS
    s_spk_cfg = *cfg;
#if CONFIG_INK_SPEAKER_MAX98357A
    if (cfg->bclk == GPIO_NUM_NC || cfg->ws == GPIO_NUM_NC || cfg->dout == GPIO_NUM_NC) {
        return ESP_OK; /* board chưa đấu loa: không đăng ký */
    }
    hw_i2s_request_tx(cfg->bclk, cfg->ws, cfg->dout);
#endif
    ink_device_t dev = { .name = "speaker", .comp = INK_COMP_AUDIO_HW, .ops = &s_spk_dev_ops, .optional = true };
    return ink_device_register(&dev);
#else
    (void)cfg;
    return ESP_OK;
#endif
}
