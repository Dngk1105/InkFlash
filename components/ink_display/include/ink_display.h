/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Chủ duy nhất của màn. Task main mượn khung, vẽ, giao khung; task màn luôn vẽ
 * khung mới nhất, bỏ khung giữa. Khung theo bố cục của interface màn
 * (ink_hal_display.h), cỡ lấy từ bảng khả năng của driver.
 */
#include "esp_err.h"
#include "ink_hal_display.h"
#include <stddef.h>
#include <stdint.h>

/* ---- Kiểu ---- */

/** Cài đặt đổi được lúc chạy. */
typedef struct {
    size_t   full_profile;         /**< index trong capabilities.profiles */
    size_t   partial_profile;      /**< index trong capabilities.profiles */
    uint16_t partials_before_full; /**< ngưỡng bóng mờ: số lần partial liên tiếp rồi full */
    uint8_t  edge_margin;          /**< đẩy lại viền rộng chừng này điểm quanh vùng đổi; 0 = tắt */
} ink_display_options_t;

typedef enum : uint8_t {
    INK_DISPLAY_MODE_NONE, /**< khung giống hệt màn đang hiện, không vẽ */
    INK_DISPLAY_MODE_PARTIAL,
    INK_DISPLAY_MODE_FULL,
} ink_display_mode_t;

/** Một lần task màn xử lý khung; mốc thời gian theo esp_timer (µs). */
typedef struct {
    esp_err_t                error; /**< ESP_OK, hoặc lỗi của policy hay driver */
    ink_display_mode_t       mode;
    ink_hal_display_rect_t   window;
    size_t                   profile;             /**< profile của slot đã vẽ */
    int8_t                   temperature_celsius; /**< đọc trước lần full; chỉ đúng khi temperature_valid */
    bool                     temperature_valid;
    uint32_t                 press_ms;   /**< thời điểm bấm bên gọi đưa vào lúc giao, theo ink_platform_now_ms() */
    int64_t                  acquire_us; /**< mượn khung */
    int64_t                  submit_us;  /**< giao khung */
    int64_t                  take_us;    /**< task màn lấy khung */
    int64_t                  diff_us;    /**< policy và mặt phẳng cũ xong */
    ink_hal_display_timing_t driver;     /**< thời gian driver trả về */
} ink_display_report_t;

/** Chạy trên task màn, ngay sau mỗi khung; không chặn lâu. */
typedef void (*ink_display_drawn_callback_t)(const ink_display_report_t *report, void *user_data);

typedef struct {
    const ink_hal_display_device_t *device;     /**< được chép */
    const uint8_t                  *background; /**< màn nền vẽ trước khi ngủ sâu; nullptr khi khởi động nguội */
    const ink_display_options_t    *options;    /**< nullptr = ink_display_default_options() */
    ink_display_drawn_callback_t    on_drawn;   /**< có thể là nullptr */
    void                           *user_data;
} ink_display_config_t;

/* ---- API ---- */

/**
 * Cài đặt mặc định: profile mặc định của driver, ngưỡng và độ rộng viền là hằng số của display.
 * @return cài đặt mặc định; profile bằng 0 nếu capabilities là nullptr
 */
ink_display_options_t ink_display_default_options(const ink_hal_display_capabilities_t *capabilities);

/**
 * Cấp khung trong RAM nội, tạo task màn. Màn nền (nếu có) coi là khung đang hiện, để lần vẽ đầu là partial cả màn.
 * Gọi từ: task main, một lần lúc khởi động.
 * @return ESP_ERR_INVALID_STATE nếu đã chạy; ESP_ERR_INVALID_ARG nếu config nullptr, device thiếu bảng hàm, bảng khả
 *         năng sai hoặc options sai; ESP_ERR_NO_MEM nếu không cấp được khung hoặc task
 */
esp_err_t ink_display_start(const ink_display_config_t *config);

/**
 * Đổi cài đặt; task màn áp dụng trước khung sau.
 * Gọi từ: task main.
 * @return ESP_ERR_INVALID_STATE nếu chưa start; ESP_ERR_INVALID_ARG nếu options nullptr, profile ngoài danh sách,
 *         partials_before_full bằng 0 hoặc edge_margin quá lớn
 */
esp_err_t ink_display_set_options(const ink_display_options_t *options);

/**
 * Mượn một khung để vẽ. Khung đã có sẵn ảnh của lần giao gần nhất (trắng nếu chưa giao lần nào).
 * Gọi từ: task main.
 * @return nullptr nếu chưa start hoặc khung mượn trước chưa giao
 */
uint8_t *ink_display_acquire(void);

/**
 * Giao khung đã mượn, không chờ.
 * Gọi từ: task main.
 * @param press_ms thời điểm bấm theo ink_platform_now_ms(), ghi vào report
 * @return ESP_ERR_INVALID_STATE nếu chưa start hoặc chưa mượn khung; ESP_ERR_INVALID_ARG nếu `frame` không phải khung
 *         đã mượn
 */
esp_err_t ink_display_submit(uint8_t *frame, uint32_t press_ms);
