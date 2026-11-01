/* Thiết kế: docs/design/display/display.md */
#pragma once

/*
 * Driver màn WeAct 3,7" (controller UC8253, 240 × 416), làm theo interface
 * ink_hal_display. BSP khởi tạo bus SPI và truyền chân qua config. Bộ nhớ do
 * bên gọi cấp, phải nằm trong RAM nội vì DMA đọc nó.
 */
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ink_hal_display.h"
#include <stdint.h>

#define INK_DRV_UC8253_WIDTH         240
#define INK_DRV_UC8253_HEIGHT        416
#define INK_DRV_UC8253_FRAME_BYTES   (INK_DRV_UC8253_WIDTH / 8 * INK_DRV_UC8253_HEIGHT)
#define INK_DRV_UC8253_PROFILE_COUNT 12   /* 11 preset và profile "tune" */
#define INK_DRV_UC8253_TR_BYTES      0xE9 /* một dải nhiệt độ trong OTP (waveform.md) */

typedef struct {
    spi_host_device_t host; /* bus đã khởi tạo, max_transfer_sz ≥ INK_DRV_UC8253_FRAME_BYTES */
    gpio_num_t        cs;
    gpio_num_t        dc;
    gpio_num_t        reset;
    gpio_num_t        busy;
    uint32_t          spi_clock_hz; /* 0 = 4 MHz */
} ink_drv_uc8253_config_t;

typedef enum : uint8_t {
    INK_DRV_UC8253_SOURCE_OTP_FORCED, /* sóng OTP, chọn dải bằng nhiệt độ ép (E0 02 + E5) */
    INK_DRV_UC8253_SOURCE_OTP_SENSOR, /* sóng OTP, chọn dải theo cảm biến (E0 00) */
    INK_DRV_UC8253_SOURCE_REGISTER,   /* LUT nạp qua thanh ghi (REG = 1), điện áp lấy từ dải TR8 trong OTP */
} ink_drv_uc8253_source_t;

typedef struct {
    const char             *name;
    ink_drv_uc8253_source_t source;
    uint8_t                 temperature;      /* OTP_FORCED: mã gửi kèm E5; 110 trở lên chọn TR8 */
    uint16_t                otp_drf_ms;       /* OTP: DRF đo trên BB0 (refresh.md), làm thời gian điển hình */
    uint8_t                 frames;           /* REGISTER: số khung mỗi lần đổi màu; 0 = chép nguyên bảng TR8 */
    uint8_t                 hold_frames;      /* REGISTER: xung giữ (ink_waveform_parameters_t) */
    uint16_t                frame_rate_hz;    /* REGISTER: tần số khung (lệnh PLL); 0 = của TR8 */
    uint8_t                 scans;            /* số lần DRF cho một lần vẽ, mỗi lần đẩy lại cùng cặp bit; 0 coi như 1 */
    bool                    pt_scan;          /* partial: quét gate cả ngoài window */
    uint8_t                 cdi;              /* byte lệnh 0x50: bit 1 = trắng; 0x97 viền làm mới (full), 0xD7 viền nổi */
    bool                    keep_power_on;    /* partial: không POF sau khi quét */
    bool                    allow_unbalanced; /* cho nạp LUT có LUTWK + LUTKW khác 0 */
} ink_drv_uc8253_profile_t;

typedef struct {
    ink_drv_uc8253_config_t config;

    /* Chỉ task màn đụng. */
    size_t                         slot_profiles[INK_HAL_DISPLAY_SLOT_COUNT];
    ink_drv_uc8253_profile_t       tune_profile;
    ink_hal_display_profile_info_t profile_info[INK_DRV_UC8253_PROFILE_COUNT];
    int                            loaded_profile; /* profile đang nạp trong thanh ghi controller; -1 = chưa có */
    bool                           register_mode;  /* PSR đang đặt REG = 1 */
    bool                           power_on;       /* nguồn analog đang bật (sau PON, chưa POF) */
    bool                           ram_trusted;    /* RAM ảnh khớp mặt phẳng của lần vẽ trước; false sau reset */
    bool                           ram_swapped;    /* số DRF từ lúc reset là lẻ: 0x10 đang là vùng NEW */
    bool                           needs_reset;    /* lần vẽ trước lỗi giữa chừng: không còn biết lượt */
    bool                           tr8_valid;
    uint8_t                        tr8[INK_DRV_UC8253_TR_BYTES]; /* dải TR8 đọc từ OTP: điện áp, VCOM, PLL, LUT */

    /* ink_drv_uc8253_tune ghi từ task khác, draw lấy ra. */
    portMUX_TYPE             tune_lock;
    bool                     tune_pending;
    ink_hal_display_slot_t   tune_slot;
    ink_drv_uc8253_profile_t tune_request;

    spi_device_handle_t spi;
    SemaphoreHandle_t   busy_ready; /* ISR give khi BUSY lên cao */
    StaticSemaphore_t   busy_ready_storage;
    uint8_t             window_buffer[INK_DRV_UC8253_FRAME_BYTES] __attribute__((aligned(4)));
} ink_drv_uc8253_t;

/*
 * Reset controller, gửi cấu hình panel và điền `out_device`. Không đổi ảnh trên
 * màn. Gọi một lần, từ task. Trả ESP_ERR_INVALID_ARG nếu có con trỏ NULL,
 * ESP_ERR_TIMEOUT nếu BUSY không lên, hoặc lỗi GPIO, SPI.
 */
esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config, ink_hal_display_device_t *out_device);

/*
 * Chỉnh tay: gán `profile` (tên bỏ qua, thành profile "tune") cho slot, có tác
 * dụng từ lần vẽ sau. Gọi được từ task khác task màn. LUT tự dựng
 * (source REGISTER, frames > 0) mà LUTWK + LUTKW khác 0 thì từ chối, kể cả khi
 * có allow_unbalanced. Trả ESP_ERR_INVALID_ARG nếu NULL, slot sai, thông số
 * ngoài khoảng, tần số khung không có trong bảng PLL, hoặc LUT lệch.
 */
esp_err_t ink_drv_uc8253_tune(ink_drv_uc8253_t *driver, ink_hal_display_slot_t slot, const ink_drv_uc8253_profile_t *profile);
