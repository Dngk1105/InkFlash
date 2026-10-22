#pragma once

#include "drv_display.h"
#include "ink_hal_display.h"

/* Khuôn nội bộ của một driver màn. Mỗi display_<chip>.c xuất một biến const drv_display_chip_t. */
typedef struct {
    const ink_display_ops_t *ops;                    /* đi vào HAL */
    esp_err_t (*init)(const drv_display_cfg_t *cfg); /* cấu hình bus + reset panel; gọi lại được (recover) */
} drv_display_chip_t;
