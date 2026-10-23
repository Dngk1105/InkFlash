#pragma once

#include "esp_err.h"

/* Hàm khởi động từng service, chỉ ink_service.c gọi. */
esp_err_t svc_storage_start(void);
esp_err_t svc_render_start(void);
esp_err_t svc_audio_start(void);
esp_err_t svc_input_start(void);
esp_err_t svc_transfer_start(void);
esp_err_t svc_power_start(void);
