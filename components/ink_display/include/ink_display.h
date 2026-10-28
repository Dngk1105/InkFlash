/* Thiết kế: docs/design/display-pipeline.md */
#pragma once

/*
 * Chủ duy nhất của màn. Task main mượn khung, vẽ, giao khung; task màn luôn vẽ
 * khung mới nhất, bỏ khung giữa. Khung theo bố cục của interface màn
 * (ink_hal_display.h), cỡ lấy từ bảng khả năng của driver.
 */
#include "esp_err.h"
#include "ink_hal_display.h"
#include <stdint.h>

/*
 * Cấp 3 khung trong RAM nội, tạo task màn. Gọi một lần lúc khởi động, từ task
 * main. Chép `*device`. Trả ESP_ERR_INVALID_STATE nếu đã chạy,
 * ESP_ERR_INVALID_ARG nếu device thiếu bảng hàm hoặc cỡ màn bằng 0,
 * ESP_ERR_NO_MEM nếu không cấp được khung hoặc task.
 */
esp_err_t ink_display_start(const ink_hal_display_device_t *device);

/*
 * Mượn một khung để vẽ, chỉ từ task main. Khung đã có sẵn ảnh của lần giao gần
 * nhất (trắng nếu chưa giao lần nào). Trả nullptr nếu chưa start hoặc khung
 * mượn trước chưa giao.
 */
uint8_t *ink_display_acquire(void);

/*
 * Giao khung đã mượn, không chờ; `press_ms` là thời điểm bấm theo
 * ink_platform_now_ms(), để đo. Trả ESP_ERR_INVALID_STATE nếu chưa start hoặc
 * chưa mượn khung, ESP_ERR_INVALID_ARG nếu `frame` không phải khung đã mượn.
 */
esp_err_t ink_display_submit(uint8_t *frame, uint32_t press_ms);

/*
 * Thêm lệnh console `display stats`. Gọi sau ink_display_start() và
 * ink_platform_console_init(). Trả ESP_ERR_INVALID_STATE nếu chưa start, hoặc
 * lỗi của ink_platform_console_add().
 */
esp_err_t ink_display_add_console_command(void);
