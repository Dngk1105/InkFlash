/* Thiết kế: docs/design/display-pipeline.md */
#pragma once

/*
 * Interface màn: một device cho một màn. Toạ độ theo hướng gốc của panel.
 * Khung ảnh 1 bit, bit 1 = trắng, gói theo hàng, điểm trái nhất ở bit cao nhất; mỗi hàng (width + 7) / 8 byte.
 */
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    INK_HAL_DISPLAY_MODE_FULL,    /* cả màn, sạch, chậm; bỏ qua window */
    INK_HAL_DISPLAY_MODE_PARTIAL, /* chỉ window */
} ink_hal_display_mode_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_hal_display_rect_t;

/* Thời gian một lần vẽ, micro giây. */
typedef struct {
    uint32_t spi_us;   /* gửi ảnh, trước khi panel quét */
    uint32_t panel_us; /* panel quét tới khi BUSY lên cao */
    uint32_t after_us; /* sau khi quét: tắt nguồn analog, ghi lại ảnh cũ */
} ink_hal_display_timing_t;

/* Bảng khả năng: driver khai báo nó làm được gì. */
typedef struct {
    uint16_t width; /* cỡ gốc */
    uint16_t height;
    bool     has_partial;
    uint8_t  partial_x_align;    /* mép x của window partial là bội của số này */
    uint16_t typical_full_ms;    /* gồm SPI và panel */
    uint16_t typical_partial_ms; /* 0 nếu không có partial */
} ink_hal_display_capabilities_t;

/* Bảng hàm. `context` là dữ liệu riêng của driver. */
typedef struct {
    /*
     * Gửi ảnh, cho panel quét, ngủ chờ BUSY rồi mới trả về. Chỉ task màn gọi.
     * `frame` là cả khung; driver tự cắt window và làm tròn mép x ra ngoài theo partial_x_align.
     * Trả ESP_ERR_INVALID_ARG nếu NULL hoặc window ra ngoài màn, ESP_ERR_NOT_SUPPORTED nếu không có kiểu vẽ đó,
     * ESP_ERR_TIMEOUT nếu BUSY không lên, hoặc lỗi SPI.
     */
    esp_err_t (*draw)(void *context, const uint8_t *frame, ink_hal_display_rect_t window, ink_hal_display_mode_t mode,
                      ink_hal_display_timing_t *out_timing);
} ink_hal_display_operations_t;

typedef struct {
    const ink_hal_display_operations_t *operations;
    void                               *context;
    ink_hal_display_capabilities_t      capabilities;
} ink_hal_display_device_t;
