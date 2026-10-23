#pragma once

#include <stdint.h>
#include "esp_err.h"

/* Cấu hình bền trong NVS, namespace "ink". Đọc khoá chưa có thì trả giá trị mặc định. */
esp_err_t ink_config_init(void);
esp_err_t ink_config_get_u32(const char *key, uint32_t *out, uint32_t def);
esp_err_t ink_config_set_u32(const char *key, uint32_t value);
