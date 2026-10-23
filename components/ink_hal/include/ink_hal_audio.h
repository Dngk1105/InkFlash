#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL âm thanh: loa (audio_out) và mic (audio_in) là hai khối riêng, đổi chip độc lập.
 * PCM chuẩn: 16 bit có dấu, mono, little-endian, INK_AUDIO_RATE_HZ. Driver tự đổi sang khung I2S của chip.
 * Âm lượng do service quyết định (nhân mẫu trước khi ghi); driver có thể có thêm set_volume phần cứng.
 */
#define INK_AUDIO_RATE_HZ 16000

typedef struct {
    esp_err_t (*start)(void *ctx, uint32_t rate_hz); /* bật amp, bật kênh I2S */
    esp_err_t (*write)(void *ctx, const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms);
    esp_err_t (*stop)(void *ctx); /* tắt amp để tiết kiệm điện */
} ink_audio_out_ops_t;

typedef struct {
    esp_err_t (*start)(void *ctx, uint32_t rate_hz);
    esp_err_t (*read)(void *ctx, int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms);
    esp_err_t (*stop)(void *ctx);
} ink_audio_in_ops_t;

/* Phía driver */
esp_err_t ink_audio_out_attach(const ink_audio_out_ops_t *ops, void *ctx);
esp_err_t ink_audio_in_attach(const ink_audio_in_ops_t *ops, void *ctx);

/* Phía service */
bool ink_audio_out_present(void);
esp_err_t ink_audio_out_start(void);
esp_err_t ink_audio_out_write(const int16_t *buf, size_t n, size_t *done, uint32_t timeout_ms);
esp_err_t ink_audio_out_stop(void);

bool ink_audio_in_present(void);
esp_err_t ink_audio_in_start(void);
esp_err_t ink_audio_in_read(int16_t *buf, size_t n, size_t *got, uint32_t timeout_ms);
esp_err_t ink_audio_in_stop(void);
