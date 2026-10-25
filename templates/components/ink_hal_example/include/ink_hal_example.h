/* Thiết kế: docs/design/hardware-variants.md */
#pragma once

/*
 * Mẫu interface phần cứng (HAL). Tầng trên chỉ thấy bảng hàm và bảng khả năng, không biết tên chip.
 * Mỗi driver điền một ink_hal_example_dev_t.
 */
#include <stdbool.h>
#include "esp_err.h"

/* Bảng khả năng: driver khai báo nó làm được gì. Chỉ thêm trường khi tầng trên thật sự dùng. */
typedef struct {
    bool has_irq; /* báo thay đổi bằng ngắt, không cần hỏi liên tục */
} ink_hal_example_caps_t;

/* Bảng hàm. `ctx` là dữ liệu riêng của driver. */
typedef struct {
    esp_err_t (*read)(void *ctx, bool *out_active);
} ink_hal_example_ops_t;

typedef struct {
    const ink_hal_example_ops_t *ops;
    void                        *ctx;
    ink_hal_example_caps_t       caps;
} ink_hal_example_dev_t;
