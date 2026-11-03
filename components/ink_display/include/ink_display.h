/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Chủ duy nhất của màn. Task main mượn khung, vẽ, giao khung kèm ý định vẽ; task màn luôn vẽ khung mới nhất, bỏ
 * khung giữa, chọn preset và profile, dọn màn khi nghỉ, tắt nguồn màn. Khung theo bố cục của interface màn
 * (ink_hal_display.h), cỡ lấy từ bảng khả năng của driver.
 */
#include "esp_err.h"
#include "ink_display_policy.h"
#include "ink_hal_display.h"
#include <stddef.h>
#include <stdint.h>

/* ---- Ý định và cờ của khung (giá trị của ink_display_policy) ---- */

typedef ink_display_policy_intent_t ink_display_intent_t;

#define INK_DISPLAY_INTENT_AUTO INK_DISPLAY_POLICY_INTENT_AUTO
#define INK_DISPLAY_INTENT_TEXT INK_DISPLAY_POLICY_INTENT_TEXT /**< trang chữ người dùng sẽ đọc */
#define INK_DISPLAY_INTENT_UI   INK_DISPLAY_POLICY_INTENT_UI   /**< con trỏ, nút: nhanh, chịu xám nhẹ */
#define INK_DISPLAY_INTENT_PAGE INK_DISPLAY_POLICY_INTENT_PAGE /**< đổi cả màn */

#define INK_DISPLAY_FLAG_DEFER_CLEANUP INK_DISPLAY_POLICY_FLAG_DEFER_CLEANUP
#define INK_DISPLAY_FLAG_BEFORE_SLEEP  INK_DISPLAY_POLICY_FLAG_BEFORE_SLEEP

/* ---- Cài đặt ---- */

/** Preset của một mục đích vẽ. */
typedef struct {
    ink_hal_display_band_t        bands[INK_HAL_DISPLAY_MAX_BANDS]; /**< xét từ đầu; id không có thì dùng mặc định của driver */
    uint8_t                       band_count;
    ink_hal_display_draw_config_t config;
    uint8_t                       debt_weight; /**< chỉ TEXT, UI, PAGE dùng */
    bool                          edge_flags;  /**< chỉ TEXT, UI, PAGE dùng: khung vẽ bằng preset này đánh dấu cờ viền */
} ink_display_preset_t;

/** Cài đặt đổi được lúc chạy. */
typedef struct {
    ink_display_preset_t presets[INK_HAL_DISPLAY_PURPOSE_COUNT];
    uint8_t              auto_ui_percent;
    uint8_t              debt_soft;
    uint8_t              debt_page;
    uint8_t              debt_ceiling;
    uint8_t              debt_ceiling_max;
    uint32_t             edge_ms; /**< bậc 1 của thang dọn; 0 = bỏ */
    uint32_t             light_ms;
    uint32_t             strong_ms;
    uint32_t             power_off_ms; /**< không có DRF chừng này thì tắt nguồn analog; 0 = chỉ tắt trước khi ngủ */
} ink_display_options_t;

/* ---- Báo cáo ---- */

/** Việc task màn vừa làm; trùng giá trị ink_display_policy_action_t, thêm tắt nguồn. */
typedef enum : uint8_t {
    INK_DISPLAY_ACTION_NONE                = INK_DISPLAY_POLICY_ACTION_NONE,
    INK_DISPLAY_ACTION_DRAW                = INK_DISPLAY_POLICY_ACTION_DRAW,
    INK_DISPLAY_ACTION_CLEAN_STRONG        = INK_DISPLAY_POLICY_ACTION_CLEAN_STRONG,
    INK_DISPLAY_ACTION_CLEAN_FAST          = INK_DISPLAY_POLICY_ACTION_CLEAN_FAST,
    INK_DISPLAY_ACTION_CLEAN_EDGE_CENTER   = INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_CENTER,
    INK_DISPLAY_ACTION_CLEAN_EDGE_NEIGHBOR = INK_DISPLAY_POLICY_ACTION_CLEAN_EDGE_NEIGHBOR,
    INK_DISPLAY_ACTION_CLEAN_LIGHT         = INK_DISPLAY_POLICY_ACTION_CLEAN_LIGHT,
    INK_DISPLAY_ACTION_POWER_OFF,
} ink_display_action_t;

/** Một lần task màn làm việc với màn; mốc thời gian theo esp_timer (µs). */
typedef struct {
    esp_err_t                 error; /**< ESP_OK, hoặc lỗi của policy hay driver */
    ink_display_action_t      action;
    bool                      frame;   /**< lần vẽ một khung đã giao; false = lượt dọn hoặc tắt nguồn */
    ink_hal_display_purpose_t purpose; /**< INK_HAL_DISPLAY_PURPOSE_COUNT khi không vẽ */
    uint16_t                  profile_id;
    ink_hal_display_rect_t    window;
    int8_t                    temperature_celsius; /**< chỉ đúng khi temperature_valid */
    bool                      temperature_valid;   /**< vừa đọc cảm biến (trước lần dọn mạnh, dọn nhanh) */
    uint8_t                   max_debt;            /**< sau lần này */
    uint8_t                   ceiling;
    uint32_t                  flagged_count;
    uint32_t                  press_ms;   /**< thời điểm bấm bên gọi đưa vào lúc giao, theo ink_platform_now_ms() */
    int64_t                   acquire_us; /**< mượn khung */
    int64_t                   submit_us;  /**< giao khung */
    int64_t                   take_us;    /**< task màn lấy khung, hoặc bắt đầu lượt dọn */
    int64_t                   diff_us;    /**< policy và mặt phẳng cũ xong */
    ink_hal_display_timing_t  driver;     /**< thời gian driver trả về */
} ink_display_report_t;

/** Chạy trên task màn, ngay sau mỗi lần làm việc với màn; không chặn lâu. */
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
 * Cài đặt mặc định: preset lấy từ mặc định theo mục đích của driver; trọng số, ngưỡng và thời gian là hằng số của display.
 * @return cài đặt mặc định; preset trống nếu capabilities là nullptr
 */
ink_display_options_t ink_display_default_options(const ink_hal_display_capabilities_t *capabilities);

/**
 * Cấp khung trong RAM nội và trạng thái từng điểm trong PSRAM, tạo task màn. Màn nền (nếu có) coi là khung đang hiện.
 * Gọi từ: task main, một lần lúc khởi động.
 * @return ESP_ERR_INVALID_STATE nếu đã chạy; ESP_ERR_INVALID_ARG nếu config nullptr, device thiếu bảng hàm, bảng khả
 *         năng sai hoặc options sai; ESP_ERR_NO_MEM nếu không cấp được bộ nhớ hoặc task
 */
esp_err_t ink_display_start(const ink_display_config_t *config);

/**
 * Đổi cài đặt; task màn áp dụng trước khung sau.
 * Gọi từ: task main.
 * @return ESP_ERR_INVALID_STATE nếu chưa start; ESP_ERR_INVALID_ARG nếu options nullptr hoặc sai (số dải, trọng số,
 *         ngưỡng nợ)
 */
esp_err_t ink_display_set_options(const ink_display_options_t *options);

/**
 * Mượn một khung để vẽ. Khung đã có sẵn ảnh của lần giao gần nhất (trắng nếu chưa giao lần nào).
 * Gọi từ: task main.
 * @return nullptr nếu chưa start hoặc khung mượn trước chưa giao
 */
uint8_t *ink_display_acquire(void);

/**
 * Giao khung đã mượn kèm ý định vẽ và cờ, không chờ.
 * Gọi từ: task main.
 * @param press_ms thời điểm bấm theo ink_platform_now_ms(), ghi vào report
 * @param flags INK_DISPLAY_FLAG_*
 * @return ESP_ERR_INVALID_STATE nếu chưa start hoặc chưa mượn khung; ESP_ERR_INVALID_ARG nếu `frame` không phải khung
 *         đã mượn hoặc ý định ngoài enum
 */
esp_err_t ink_display_submit(uint8_t *frame, uint32_t press_ms, ink_display_intent_t intent, uint8_t flags);
