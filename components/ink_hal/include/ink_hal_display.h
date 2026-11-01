/* Thiết kế: docs/design/display/display.md */
#pragma once

/* Mỗi pixel 1 bit black/white (white = bit 1)
 * Tọa độ theo hướng gốc của panel
 * Khung theo từng hàng, điểm trái nhất ở bit cao nhất của byte
 * Chỉ Display task gọi các hàm trong operations
 */
#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_hal_display_rect_t;

typedef enum : uint8_t {
    INK_HAL_DISPLAY_SLOT_FULL,
    INK_HAL_DISPLAY_SLOT_PARTIAL,
    INK_HAL_DISPLAY_SLOT_COUNT,
} ink_hal_display_slot_t;

typedef struct {
    uint32_t spi_us;   /* Gửi ảnh và bảng sóng, trước khi quét*/
    uint32_t panel_us; /* panel quét tới khi BUSY cao (rảnh)*/
    uint32_t after_us; /* sau khi quét: tắt nguồn */
} ink_hal_display_timing_t;

typedef struct {
    const char *name;
    uint32_t    typical_ms;
} ink_hal_display_profile_info_t;

typedef struct {
    uint16_t                              width; /* Cỡ gốc panel*/
    uint16_t                              height;
    uint8_t                               x_align; /* Mép trái/phải là bội của x_align*/
    const ink_hal_display_profile_info_t *profiles;
    size_t                                profile_count;
    size_t                                default_profiles[INK_HAL_DISPLAY_SLOT_COUNT]; /* profile gán lúc khởi tạo */
} ink_hal_display_capabilities_t;

typedef struct {
    /*
     * Vẽ window theo cặp bit (mới, cũ) của từng điểm, chờ panel quét xong. Hai
     * mặt phẳng là con trỏ tới cả khung, nằm trong RAM nội. Driver làm tròn mép x
     * ra ngoài theo x_align, và tự nới ra cả màn ở lần vẽ đầu sau reset.
     */
    esp_err_t (*draw)(void *context, const uint8_t *plane_new, const uint8_t *plane_old, ink_hal_display_rect_t window,
                      ink_hal_display_slot_t slot, ink_hal_display_timing_t *out_timing);
    /* Gán profile thứ `index` trong capabilities.profiles cho slot, có tác dụng từ
     * lần vẽ sau. */
    esp_err_t (*select_profile)(void *context, ink_hal_display_slot_t slot, size_t index);
    /* Đọc cảm biến nhiệt độ của controller, đơn vị Celsius. */
    esp_err_t (*read_temperature)(void *context, int8_t *out_celsius);
} ink_hal_display_operations_t;

typedef struct {
    const ink_hal_display_operations_t *operations;
    void                               *context;
    ink_hal_display_capabilities_t      capabilities;
} ink_hal_display_device_t;
