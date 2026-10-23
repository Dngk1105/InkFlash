#pragma once

/*
 * Chọn bảng chân theo board (Kconfig INK_BOARD), điền mặc định, rồi kiểm tra lúc biên dịch.
 * Chỉ BSP và driver được include file này. Service không được include (CMake không cho).
 */
#include "sdkconfig.h"
#include "driver/gpio.h"

#if CONFIG_INK_BOARD_BREADBOARD_GD0
#include "boards/breadboard_gd0.h"
#elif CONFIG_INK_BOARD_PCB
#include "boards/pcb.h"
#else
#error "Chưa chọn board (INK_BOARD)"
#endif

/* Chân nào board không khai báo = chưa nối. */
#ifndef BSP_BTN_A
#define BSP_BTN_A GPIO_NUM_NC
#endif
#ifndef BSP_BTN_B
#define BSP_BTN_B GPIO_NUM_NC
#endif
#ifndef BSP_BTN_ACTIVE_LOW
#define BSP_BTN_ACTIVE_LOW 1
#endif
#ifndef BSP_EPD_SCK
#define BSP_EPD_SCK GPIO_NUM_NC
#endif
#ifndef BSP_EPD_MOSI
#define BSP_EPD_MOSI GPIO_NUM_NC
#endif
#ifndef BSP_EPD_CS
#define BSP_EPD_CS GPIO_NUM_NC
#endif
#ifndef BSP_EPD_DC
#define BSP_EPD_DC GPIO_NUM_NC
#endif
#ifndef BSP_EPD_RST
#define BSP_EPD_RST GPIO_NUM_NC
#endif
#ifndef BSP_EPD_BUSY
#define BSP_EPD_BUSY GPIO_NUM_NC
#endif
#ifndef BSP_I2C_SDA
#define BSP_I2C_SDA GPIO_NUM_NC
#endif
#ifndef BSP_I2C_SCL
#define BSP_I2C_SCL GPIO_NUM_NC
#endif
#ifndef BSP_IMU_INT1
#define BSP_IMU_INT1 GPIO_NUM_NC
#endif
#ifndef BSP_I2C_FREQ_HZ
#define BSP_I2C_FREQ_HZ 100000
#endif
#ifndef BSP_RGB_LED
#define BSP_RGB_LED GPIO_NUM_NC
#endif
#ifndef BSP_RGB_BRIGHTNESS
#define BSP_RGB_BRIGHTNESS 255
#endif
#ifndef BSP_EPD_ROTATION
#define BSP_EPD_ROTATION 0
#endif
#ifndef BSP_MIC_BCLK
#define BSP_MIC_BCLK GPIO_NUM_NC
#endif
#ifndef BSP_MIC_WS
#define BSP_MIC_WS GPIO_NUM_NC
#endif
#ifndef BSP_MIC_DIN
#define BSP_MIC_DIN GPIO_NUM_NC
#endif
#ifndef BSP_SPK_BCLK
#define BSP_SPK_BCLK GPIO_NUM_NC
#endif
#ifndef BSP_SPK_WS
#define BSP_SPK_WS GPIO_NUM_NC
#endif
#ifndef BSP_SPK_DOUT
#define BSP_SPK_DOUT GPIO_NUM_NC
#endif
#ifndef BSP_SPK_SD
#define BSP_SPK_SD GPIO_NUM_NC
#endif
#ifndef BSP_CHG_INT
#define BSP_CHG_INT GPIO_NUM_NC
#endif
#ifndef BSP_BAT_ADC
#define BSP_BAT_ADC GPIO_NUM_NC
#endif
#ifndef BSP_RAIL_DISPLAY_EN
#define BSP_RAIL_DISPLAY_EN GPIO_NUM_NC
#endif
#ifndef BSP_RAIL_AUDIO_EN
#define BSP_RAIL_AUDIO_EN GPIO_NUM_NC
#endif
#ifndef BSP_RAIL_SENSORS_EN
#define BSP_RAIL_SENSORS_EN GPIO_NUM_NC
#endif

/* Danh sách mọi chân: dùng cho static assert và kiểm tra trùng chân lúc khởi động. */
#define BSP_PIN_LIST(X)                                                               \
    X(BTN_A, BSP_BTN_A) X(BTN_B, BSP_BTN_B)                                             \
    X(EPD_SCK, BSP_EPD_SCK) X(EPD_MOSI, BSP_EPD_MOSI) X(EPD_CS, BSP_EPD_CS)             \
    X(EPD_DC, BSP_EPD_DC) X(EPD_RST, BSP_EPD_RST) X(EPD_BUSY, BSP_EPD_BUSY)             \
    X(I2C_SDA, BSP_I2C_SDA) X(I2C_SCL, BSP_I2C_SCL) X(IMU_INT1, BSP_IMU_INT1)           \
    X(MIC_BCLK, BSP_MIC_BCLK) X(MIC_WS, BSP_MIC_WS) X(MIC_DIN, BSP_MIC_DIN)             \
    X(SPK_BCLK, BSP_SPK_BCLK) X(SPK_WS, BSP_SPK_WS) X(SPK_DOUT, BSP_SPK_DOUT)           \
    X(SPK_SD, BSP_SPK_SD) X(CHG_INT, BSP_CHG_INT) X(BAT_ADC, BSP_BAT_ADC)               \
    X(RAIL_DISPLAY_EN, BSP_RAIL_DISPLAY_EN) X(RAIL_AUDIO_EN, BSP_RAIL_AUDIO_EN)         \
    X(RAIL_SENSORS_EN, BSP_RAIL_SENSORS_EN) X(RGB_LED, BSP_RGB_LED)

/* Các cặp được phép dùng chung một chân (bus I2S chung, một công tắc tải cho nhiều khối). */
#define BSP_PIN_SHARE_LIST(X)                                                           \
    X(MIC_BCLK, SPK_BCLK) X(MIC_WS, SPK_WS)                                             \
    X(RAIL_DISPLAY_EN, RAIL_AUDIO_EN) X(RAIL_DISPLAY_EN, RAIL_SENSORS_EN)               \
    X(RAIL_AUDIO_EN, RAIL_SENSORS_EN)

/*
 * Chân hợp lệ trên ESP32-S3-WROOM-1-N16R8: GPIO0–21, 38–48.
 * Cấm: GPIO22–25 (không tồn tại), GPIO26–32 (flash), GPIO33–37 (PSRAM octal).
 */
#define BSP_PIN_VALID(p) ((p) == GPIO_NUM_NC || (((p) >= 0 && (p) <= 21) || ((p) >= 38 && (p) <= 48)))
/* Chân đánh thức deep sleep phải là RTC GPIO (GPIO0–21). */
#define BSP_PIN_IS_RTC(p) ((p) >= 0 && (p) <= 21)
/* ADC2 (GPIO11–20) tranh với WiFi, nên đo pin phải dùng ADC1 (GPIO1–10). */
#define BSP_PIN_IS_ADC1(p) ((p) >= 1 && (p) <= 10)

#define BSP_ASSERT_VALID_(name, pin) \
    _Static_assert(BSP_PIN_VALID(pin), "bsp: chan " #name " nam trong vung cam (GPIO22-37) hoac ngoai 0-48");
BSP_PIN_LIST(BSP_ASSERT_VALID_)
#undef BSP_ASSERT_VALID_

/*
 * PCB: "bấm nút thì thức" nên cả hai nút phải là RTC GPIO. Breadboard được phép lệch (nút không phải RTC chỉ
 * không đánh thức được; BSP ghi cảnh báo lúc khởi động và chỉ đưa chân RTC vào wake_mask).
 */
#if CONFIG_INK_BOARD_PCB
_Static_assert(BSP_BTN_A != GPIO_NUM_NC && BSP_PIN_IS_RTC(BSP_BTN_A), "bsp: PCB: BTN_A phai la RTC GPIO (0-21)");
_Static_assert(BSP_BTN_B != GPIO_NUM_NC && BSP_PIN_IS_RTC(BSP_BTN_B), "bsp: PCB: BTN_B phai la RTC GPIO (0-21)");
#endif
_Static_assert(BSP_BAT_ADC == GPIO_NUM_NC || BSP_PIN_IS_ADC1(BSP_BAT_ADC), "bsp: BAT_ADC phai la ADC1 (GPIO1-10)");
