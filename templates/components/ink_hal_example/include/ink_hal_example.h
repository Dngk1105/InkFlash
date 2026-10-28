/* Thiết kế: docs/design/hardware-variants.md */
#pragma once

/*
 * Mẫu interface phần cứng (HAL). Tầng trên chỉ thấy bảng hàm và bảng khả năng,
 * không biết tên chip. Mỗi driver điền một ink_hal_example_device_t.
 */
#include "esp_err.h"
#include <stdbool.h>

/* Bảng khả năng: driver khai báo nó làm được gì. Chỉ thêm trường khi tầng trên
 * thật sự dùng. */
typedef struct {
  bool has_interrupt; /* báo thay đổi bằng ngắt, không cần hỏi liên tục */
} ink_hal_example_capabilities_t;

/* Bảng hàm. `context` là dữ liệu riêng của driver. */
typedef struct {
  esp_err_t (*read)(void *context, bool *out_active);
} ink_hal_example_operations_t;

typedef struct {
  const ink_hal_example_operations_t *operations;
  void *context;
  ink_hal_example_capabilities_t capabilities;
} ink_hal_example_device_t;
