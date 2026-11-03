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

#define INK_HAL_DISPLAY_SOURCE_HAND 0xFF /**< profile vẽ tay; 0–8 là dải TR0–TR8 chép từ OTP */
#define INK_HAL_DISPLAY_MAX_BANDS   9

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ink_hal_display_rect_t;

/** Mục đích vẽ; display giữ một preset cho mỗi mục đích. Chỉ thêm vào cuối: cài đặt đã lưu theo thứ tự này. */
typedef enum : uint8_t {
    INK_HAL_DISPLAY_PURPOSE_TEXT,
    INK_HAL_DISPLAY_PURPOSE_UI,
    INK_HAL_DISPLAY_PURPOSE_PAGE,
    INK_HAL_DISPLAY_PURPOSE_CLEAN_STRONG,
    INK_HAL_DISPLAY_PURPOSE_CLEAN_FAST,
    INK_HAL_DISPLAY_PURPOSE_CLEAN_EDGE_CENTER,
    INK_HAL_DISPLAY_PURPOSE_CLEAN_EDGE_NEIGHBOR,
    INK_HAL_DISPLAY_PURPOSE_CLEAN_LIGHT,
    INK_HAL_DISPLAY_PURPOSE_COUNT,
} ink_hal_display_purpose_t;

/** Cấu hình đi kèm mỗi lần vẽ, tách khỏi profile để một bảng sóng dùng được cho nhiều mục đích. */
typedef struct {
    bool partial_mode; /**< vào chế độ partial (PTL) với window; false = vẽ cả màn, bỏ qua window */
    bool pt_scan;      /**< chế độ partial: quét gate cả ngoài window */
    bool drive_border; /**< kéo viền màn theo bảng sóng; false = viền nổi */
} ink_hal_display_draw_config_t;

typedef struct {
    uint32_t spi_us;   /**< Gửi ảnh và bảng sóng, trước khi quét */
    uint32_t power_us; /**< bật nguồn analog (PON) nếu đang tắt; 0 nếu nguồn đã bật */
    uint32_t panel_us; /**< panel quét tới khi BUSY cao (rảnh), không gồm power_us */
    uint32_t after_us; /**< sau khi quét */
} ink_hal_display_timing_t;

typedef struct {
    uint16_t    id; /**< cố định giữa các bản build; cài đặt lưu id */
    const char *name;
    uint8_t     source;  /**< 0–8 = TR0–TR8, INK_HAL_DISPLAY_SOURCE_HAND */
    uint32_t    draw_ms; /**< một lần vẽ (mọi lần quét), script tính */
} ink_hal_display_profile_info_t;

/** Một dải nhiệt độ: nhiệt độ ≤ max_celsius thì dùng profile_id. */
typedef struct {
    int8_t   max_celsius;
    uint16_t profile_id;
} ink_hal_display_band_t;

/** Mặc định của driver cho một mục đích vẽ. */
typedef struct {
    const ink_hal_display_band_t *bands; /**< xét từ đầu; nhiệt độ lớn hơn dải cuối thì dùng dải cuối */
    uint8_t                       band_count;
    ink_hal_display_draw_config_t config;
} ink_hal_display_purpose_default_t;

typedef struct {
    uint16_t                                 width; /**< Cỡ gốc panel */
    uint16_t                                 height;
    uint8_t                                  x_align;  /**< Mép trái/phải là bội của x_align */
    const ink_hal_display_profile_info_t    *profiles; /**< driver có thể đổi phần tử lúc chạy (profile tạm) */
    size_t                                   profile_count;
    const ink_hal_display_purpose_default_t *purpose_defaults; /**< INK_HAL_DISPLAY_PURPOSE_COUNT phần tử */
} ink_hal_display_capabilities_t;

typedef struct {
    /**
     * Vẽ window theo cặp bit (mới, cũ) của từng điểm bằng profile thứ `profile`, chờ panel quét xong.
     * Bật nguồn analog nếu đang tắt và để bật sau khi quét.
     * Driver làm tròn mép x ra ngoài theo x_align, và tự nới ra cả màn ở lần vẽ đầu sau reset.
     * Gọi từ: task màn.
     * @param plane_new, plane_old con trỏ tới cả khung, nằm trong RAM nội
     * @return ESP_ERR_INVALID_ARG nếu tham số sai hoặc profile ngoài danh sách; ESP_ERR_INVALID_STATE nếu profile chưa
     *         nạp (profile tạm); ESP_ERR_TIMEOUT nếu BUSY không lên; lỗi SPI
     */
    esp_err_t (*draw)(void *context, const uint8_t *plane_new, const uint8_t *plane_old, ink_hal_display_rect_t window, size_t profile,
                      const ink_hal_display_draw_config_t *config, ink_hal_display_timing_t *out_timing);
    /**
     * Tắt nguồn analog nếu đang bật.
     * Gọi từ: task màn.
     * @return ESP_ERR_TIMEOUT nếu BUSY không lên; lỗi SPI
     */
    esp_err_t (*power_off)(void *context);
    /**
     * Đọc cảm biến nhiệt độ của controller, đơn vị °C.
     * Gọi từ: task màn.
     */
    esp_err_t (*read_temperature)(void *context, int8_t *out_celsius);
} ink_hal_display_operations_t;

typedef struct {
    const ink_hal_display_operations_t *operations;
    void                               *context;
    ink_hal_display_capabilities_t      capabilities;
} ink_hal_display_device_t;
