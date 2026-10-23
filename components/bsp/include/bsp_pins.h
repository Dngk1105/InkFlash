#pragma once

/*
 * Bảng chân của board đang chọn + kiểm tra lúc biên dịch.
 * Chỉ BSP và driver được include file này; service/app không thấy (CMake không cho).
 */
#include "sdkconfig.h"
#include "driver/gpio.h"

#if CONFIG_INK_BOARD_BREADBOARD_GD0
#include "boards/breadboard_gd0.h"
#elif CONFIG_INK_BOARD_PCB_V1
#include "boards/pcb_v1.h"
#else
#error "Chưa chọn board (INK_BOARD)"
#endif

/* Chân board không khai báo = chưa nối */
#define BSP_PIN_DEFAULT_LIST(X)                                                          \
    X(BSP_BTN_A) X(BSP_BTN_B) X(BSP_EPD_SCK) X(BSP_EPD_MOSI) X(BSP_EPD_CS) X(BSP_EPD_DC)  \
    X(BSP_EPD_RST) X(BSP_EPD_BUSY) X(BSP_I2C_SDA) X(BSP_I2C_SCL) X(BSP_I2S_BCLK)          \
    X(BSP_I2S_WS) X(BSP_I2S_MIC_DIN) X(BSP_I2S_SPK_DOUT) X(BSP_RGB_LED)
#ifndef BSP_BTN_B
#define BSP_BTN_B GPIO_NUM_NC
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
#ifndef BSP_EPD_ROTATION
#define BSP_EPD_ROTATION 0
#endif
#ifndef BSP_I2C_SDA
#define BSP_I2C_SDA GPIO_NUM_NC
#endif
#ifndef BSP_I2C_SCL
#define BSP_I2C_SCL GPIO_NUM_NC
#endif
#ifndef BSP_I2C_FREQ_HZ
#define BSP_I2C_FREQ_HZ 100000
#endif
#ifndef BSP_I2S_BCLK
#define BSP_I2S_BCLK GPIO_NUM_NC
#endif
#ifndef BSP_I2S_WS
#define BSP_I2S_WS GPIO_NUM_NC
#endif
#ifndef BSP_I2S_MIC_DIN
#define BSP_I2S_MIC_DIN GPIO_NUM_NC
#endif
#ifndef BSP_I2S_SPK_DOUT
#define BSP_I2S_SPK_DOUT GPIO_NUM_NC
#endif
#ifndef BSP_RGB_LED
#define BSP_RGB_LED GPIO_NUM_NC
#endif

/*
 * ESP32-S3-WROOM-1-N16R8: dùng được GPIO0–21 và 38–48.
 * Cấm GPIO22–25 (không tồn tại), 26–32 (flash), 33–37 (PSRAM octal).
 */
#define BSP_PIN_VALID(p)  ((p) == GPIO_NUM_NC || ((p) >= 0 && (p) <= 21) || ((p) >= 38 && (p) <= 48))
#define BSP_PIN_IS_RTC(p) ((p) >= 0 && (p) <= 21) /* đánh thức được từ deep sleep */

#define BSP_ASSERT_PIN_(p) _Static_assert(BSP_PIN_VALID(p), "bsp: " #p " o vung cam (GPIO22-37) hoac ngoai 0-48");
BSP_PIN_DEFAULT_LIST(BSP_ASSERT_PIN_)
#undef BSP_ASSERT_PIN_

#if CONFIG_INK_BOARD_PCB_V1
/* Sản phẩm: "bấm nút nào cũng thức" nên cả hai nút phải là RTC GPIO. */
_Static_assert(BSP_PIN_IS_RTC(BSP_BTN_A), "bsp: PCB: BTN_A phai la RTC GPIO (0-21)");
_Static_assert(BSP_BTN_B == GPIO_NUM_NC || BSP_PIN_IS_RTC(BSP_BTN_B), "bsp: PCB: BTN_B phai la RTC GPIO (0-21)");
#endif
