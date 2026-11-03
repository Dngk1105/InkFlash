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

#define INK_DRV_UC8253_WIDTH              240
#define INK_DRV_UC8253_HEIGHT             416
#define INK_DRV_UC8253_FRAME_BYTES        (INK_DRV_UC8253_WIDTH / 8 * INK_DRV_UC8253_HEIGHT)
#define INK_DRV_UC8253_TABLE_COUNT        5  /* LUTC, LUTWW, LUTKW, LUTWK, LUTKK (lệnh 0x20–0x24) */
#define INK_DRV_UC8253_TABLE_BYTES        42 /* 6 nhóm × 7 byte, chế độ KW */
#define INK_DRV_UC8253_NAME_BYTES         16
#define INK_DRV_UC8253_MAX_PROFILES       32                /* chỗ trong bảng khả năng, gồm profile tạm */
#define INK_DRV_UC8253_OTP_BYTES          (0x2A + 9 * 0xE9) /* bank 0 từ khoá A5 tới hết dải TR8 (waveform.md) */
#define INK_DRV_UC8253_PSR_SECOND_DEFAULT 0x0A /* TIEG = 0, NORG = 1, VC_LUTZ = 0: ít tối vùng trắng nhất khi POF (refresh.md) */
#define INK_DRV_UC8253_VCOM_FROM_PROFILE  0xFF /* không thay VCOM_DC của profile */

typedef struct {
    spi_host_device_t host; /**< bus đã khởi tạo, max_transfer_sz ≥ INK_DRV_UC8253_FRAME_BYTES */
    gpio_num_t        cs;
    gpio_num_t        dc;
    gpio_num_t        reset;
    gpio_num_t        busy;
    uint32_t          spi_clock_hz; /**< 0 = 4 MHz */
} ink_drv_uc8253_config_t;

/** Một profile bảng sóng, nạp qua thanh ghi (REG = 1). Do tools/profile_gen.py sinh. */
typedef struct {
    uint16_t    id;
    const char *name;
    uint8_t     source;   /**< 0–8 = TR0–TR8, INK_HAL_DISPLAY_SOURCE_HAND */
    uint8_t     scans;    /**< số DRF cho một lần vẽ, 1–4 */
    uint8_t     pll;      /**< byte lệnh 0x30 */
    uint8_t     power[4]; /**< VG, VSH, VSL, VDHR: byte 2–5 của lệnh 0x01 */
    uint8_t     vcom;     /**< byte lệnh 0x82 */
    uint8_t     eopt;
    uint8_t     xon[2];
    uint16_t    drf_ms; /**< một lần DRF */
    uint8_t     tables[INK_DRV_UC8253_TABLE_COUNT][INK_DRV_UC8253_TABLE_BYTES];
} ink_drv_uc8253_profile_t;

typedef struct {
    ink_drv_uc8253_config_t config;

    /** Chỉ task màn đụng. */
    const ink_drv_uc8253_profile_t *temporary_profile; /**< nullptr = chưa nạp profile tạm */
    int                             loaded_profile;    /**< profile đang nạp trong thanh ghi controller; -1 = chưa có */
    bool                            power_on;          /**< nguồn analog đang bật (sau PON, chưa POF) */
    bool                            ram_trusted;       /**< RAM ảnh khớp mặt phẳng của lần vẽ trước; false sau reset */
    bool                            ram_swapped;       /**< số DRF từ lúc reset là lẻ: 0x10 đang là vùng NEW */
    bool                            needs_reset;       /**< lần vẽ trước lỗi giữa chừng: không còn biết lượt */
    uint8_t                         psr_second;        /**< byte 2 của PSR đã gửi controller */
    uint8_t vcom_override; /**< byte lệnh 0x82 thay cho của profile; INK_DRV_UC8253_VCOM_FROM_PROFILE = không thay */

    /** Yêu cầu từ task khác, giữ request_lock: draw (và power_off với PSR) lấy ra. */
    portMUX_TYPE             request_lock;
    bool                     temporary_pending;
    ink_drv_uc8253_profile_t temporary_request;
    char                     temporary_request_name[INK_DRV_UC8253_NAME_BYTES];
    ink_drv_uc8253_profile_t temporary;
    char                     temporary_name[INK_DRV_UC8253_NAME_BYTES];
    bool                     psr_second_pending;
    uint8_t                  psr_second_request;
    bool                     vcom_override_pending;
    uint8_t                  vcom_override_request;

    ink_hal_display_profile_info_t profile_info[INK_DRV_UC8253_MAX_PROFILES];

    spi_device_handle_t spi;
    SemaphoreHandle_t   busy_ready; /**< ISR give khi BUSY lên cao */
    StaticSemaphore_t   busy_ready_storage;
    uint8_t             window_buffer[INK_DRV_UC8253_FRAME_BYTES] __attribute__((aligned(4)));
} ink_drv_uc8253_t;

/**
 * Reset controller, gửi cấu hình panel và điền `out_device`. Không đổi ảnh trên màn.
 * Bảng khả năng liệt kê các profile cố định rồi profile tạm ở cuối.
 * Gọi từ: bsp, một lần, từ task.
 * @return ESP_ERR_INVALID_ARG nếu có con trỏ nullptr; ESP_ERR_TIMEOUT nếu BUSY không lên; lỗi GPIO, SPI
 */
esp_err_t ink_drv_uc8253_init(ink_drv_uc8253_t *driver, const ink_drv_uc8253_config_t *config, ink_hal_display_device_t *out_device);

/**
 * Nạp gói `.inkw` (tools/profile_gen.py --blob) thành profile tạm, có tác dụng từ lần vẽ sau. Chỉ firmware test dùng.
 * Gọi từ: task khác task màn, hoặc task màn giữa hai lần vẽ.
 * @return ESP_ERR_INVALID_ARG nếu nullptr, sai mã nhận dạng, phiên bản, độ dài hoặc CRC; giữ profile tạm cũ
 */
esp_err_t ink_drv_uc8253_load_temporary(ink_drv_uc8253_t *driver, const uint8_t *blob, size_t length);

/**
 * Đọc INK_DRV_UC8253_OTP_BYTES byte đầu của OTP bằng lệnh 0xA2 vào `out`. Chỉ firmware test dùng.
 * Gọi từ: task đang gọi draw (không gọi song song với draw).
 * @return ESP_ERR_INVALID_ARG nếu nullptr hoặc length nhỏ hơn INK_DRV_UC8253_OTP_BYTES; lỗi SPI
 */
esp_err_t ink_drv_uc8253_read_otp(ink_drv_uc8253_t *driver, uint8_t *out, size_t length);

/**
 * Đổi byte 2 của PSR (VCMZ, TS_AUTO, TIEG, NORG, VC_LUTZ; commands.md), gửi controller trước lần vẽ hoặc tắt nguồn sau.
 * Mặc định INK_DRV_UC8253_PSR_SECOND_DEFAULT. Chỉ firmware test dùng.
 * Gọi từ: task bất kỳ.
 * @return ESP_ERR_INVALID_ARG nếu nullptr hoặc `value` có bit D7–D5
 */
esp_err_t ink_drv_uc8253_set_psr_second(ink_drv_uc8253_t *driver, uint8_t value);

/**
 * Byte 2 của PSR đã đặt gần nhất, kể cả khi chưa gửi controller.
 * Gọi từ: task bất kỳ.
 */
uint8_t ink_drv_uc8253_psr_second(ink_drv_uc8253_t *driver);

/**
 * Thay VCOM_DC (byte lệnh 0x82) của mọi profile bằng `value`, có tác dụng từ lần vẽ sau; INK_DRV_UC8253_VCOM_FROM_PROFILE
 * trả về VCOM_DC của từng profile. Chỉ firmware test dùng.
 * Gọi từ: task bất kỳ.
 * @return ESP_ERR_INVALID_ARG nếu nullptr hoặc `value` lớn hơn 0x7E mà không phải INK_DRV_UC8253_VCOM_FROM_PROFILE
 */
esp_err_t ink_drv_uc8253_set_vcom_override(ink_drv_uc8253_t *driver, uint8_t value);

/**
 * VCOM_DC thay thế đã đặt gần nhất, kể cả khi chưa gửi controller.
 * Gọi từ: task bất kỳ.
 */
uint8_t ink_drv_uc8253_vcom_override(ink_drv_uc8253_t *driver);
