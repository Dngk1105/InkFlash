#pragma once

#include <stdint.h>
#include "esp_err.h"

/*
 * Audio service: phát media của thẻ (16 kHz mono, IMA-ADPCM hoặc PCM) qua HAL audio_out, có queue phát,
 * tắt amp khi rảnh. CHƯA VIẾT — giai đoạn 5.
 */
esp_err_t audio_play_media(const char *media_hash);
esp_err_t audio_stop(void);
esp_err_t audio_set_volume(uint8_t percent);
