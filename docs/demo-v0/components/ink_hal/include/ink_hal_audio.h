#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL âm thanh: mic và loa là hai khối riêng, đổi chip độc lập (ICS-43434 ↔ INMP441, MAX98357A ↔ amp khác).
 *
 * Định dạng PCM chuẩn: 16 bit có dấu, mono, little-endian, tốc độ INK_AUDIO_RATE_HZ.
 * Driver tự đổi từ định dạng của I2S/chip (32 bit, trái/phải, dịch bit...).
 */
#define INK_AUDIO_RATE_HZ 16000

typedef struct {
    esp_err_t (*start)(void *ctx, uint32_t rate_hz);
    /* Đọc tối đa n mẫu, ghi số mẫu thật vào *got. */
    esp_err_t (*read)(void *ctx, int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms);
    esp_err_t (*stop)(void *ctx);
} ink_mic_ops_t;

typedef struct {
    esp_err_t (*start)(void *ctx, uint32_t rate_hz);
    esp_err_t (*write)(void *ctx, const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms);
    esp_err_t (*stop)(void *ctx);
    esp_err_t (*set_volume)(void *ctx, uint8_t percent); /* 0–100, có thể NULL */
} ink_speaker_ops_t;

esp_err_t ink_mic_attach(const ink_mic_ops_t *ops, void *ctx);
bool ink_mic_present(void);
esp_err_t ink_mic_start(void);
esp_err_t ink_mic_read(int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms);
esp_err_t ink_mic_stop(void);

esp_err_t ink_speaker_attach(const ink_speaker_ops_t *ops, void *ctx);
bool ink_speaker_present(void);
esp_err_t ink_speaker_start(void);
esp_err_t ink_speaker_write(const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms);
esp_err_t ink_speaker_stop(void);
esp_err_t ink_speaker_set_volume(uint8_t percent);
