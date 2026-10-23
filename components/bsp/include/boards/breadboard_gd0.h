#pragma once

/*
 * GĐ0: ESP32-S3-DevKitC-1 N16R8 trên breadboard. Nguồn: docs/hardware/pinmap.md (đấu dây 2026-10-21).
 * Chân không #define = chưa nối (GPIO_NUM_NC). Đổi chân thì sửa pinmap.md trước, rồi sửa file này.
 */

/* Nút: A = GPIO8 (RTC, đánh thức được), B = GPIO46 (không phải RTC, chân strapping) */
#define BSP_BTN_A          GPIO_NUM_8
#define BSP_BTN_B          GPIO_NUM_46
#define BSP_BTN_ACTIVE_LOW 1

/* Màn e-paper GDEY037T03 / UC8253 (SPI, chỉ ghi) */
#define BSP_EPD_SCK        GPIO_NUM_4
#define BSP_EPD_MOSI       GPIO_NUM_5
#define BSP_EPD_CS         GPIO_NUM_6
#define BSP_EPD_DC         GPIO_NUM_7
#define BSP_EPD_RST        GPIO_NUM_15
#define BSP_EPD_BUSY       GPIO_NUM_16
#define BSP_EPD_ROTATION   1 /* 416×240 nằm ngang, như demo */

/* I2C chung (hiện chỉ có BNO055) */
#define BSP_I2C_SDA        GPIO_NUM_17
#define BSP_I2C_SCL        GPIO_NUM_18
#define BSP_I2C_FREQ_HZ    100000

/* I2S chung cho mic INMP441 và amp MAX98357A */
#define BSP_I2S_BCLK       GPIO_NUM_9
#define BSP_I2S_WS         GPIO_NUM_10
#define BSP_I2S_MIC_DIN    GPIO_NUM_11
#define BSP_I2S_SPK_DOUT   GPIO_NUM_12

/* LED WS2812 trên DevKit */
#define BSP_RGB_LED        GPIO_NUM_48
