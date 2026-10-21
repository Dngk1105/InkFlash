#pragma once

/*
 * GĐ0: DevKitC-1 N16R8 trên breadboard. Nguồn: docs/hardware/pinmap.md (cột Breadboard), bảng đấu dây ngày 2026-10-21.
 * Chân nào không #define ở đây thì mặc định GPIO_NUM_NC (chưa nối): driver bỏ qua và BSP ghi cảnh báo lúc khởi động.
 * Đổi chân hay đổi chiều gắn linh kiện thì chỉ sửa file này. Danh sách đủ các chân: bsp_pins.h.
 * GPIO19/20 là USB D-/D+ (console), không dùng cho việc khác.
 */

/* Nút bấm */
#define BSP_BTN_A          GPIO_NUM_8   /* BTN1. RTC GPIO: đánh thức được deep sleep */
#define BSP_BTN_B          GPIO_NUM_46  /* BTN2. KHÔNG phải RTC GPIO: không đánh thức được. Chân strapping, xem pinmap.md */
#define BSP_BTN_ACTIVE_LOW 1            /* CHƯA XÁC NHẬN: giả định cả 2 nút nối xuống GND, kéo lên nội */

/* Màn e-paper WeAct (SPI) */
#define BSP_EPD_SCK        GPIO_NUM_4   /* module ghi: SCL */
#define BSP_EPD_MOSI       GPIO_NUM_5   /* module ghi: SDA */
#define BSP_EPD_CS         GPIO_NUM_6
#define BSP_EPD_DC         GPIO_NUM_7
#define BSP_EPD_RST        GPIO_NUM_15
#define BSP_EPD_BUSY       GPIO_NUM_16
#define BSP_EPD_ROTATION   1            /* như demo: setRotation(1) → 416×240 nằm ngang */

/* Màn không dùng MISO (chỉ ghi). */

/* I2C: hiện chỉ có BNO055 */
#define BSP_I2C_SDA        GPIO_NUM_17
#define BSP_I2C_SCL        GPIO_NUM_18
#define BSP_I2C_FREQ_HZ    100000

/* LED RGB (WS2812) trên DevKit. */
#define BSP_RGB_LED        GPIO_NUM_48
#define BSP_RGB_BRIGHTNESS 50           /* như demo: setBrightness(50) */

/* IMU: BNO055 (thay tạm cho LSM6DSO/LIS2DH12 trên breadboard) */
/* #define BSP_IMU_INT1       chưa đấu */
#define BSP_IMU_ADDR       0            /* 0 = địa chỉ mặc định của chip */
/* Chip → thiết bị, ma trận xoay 3×3. CHƯA XÁC NHẬN hướng gắn BNO055: tạm là ma trận đơn vị. */
#define BSP_IMU_MOUNT      { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }

/* Âm thanh: mic INMP441 và loa MAX98357A dùng chung BCLK/WS */
#define BSP_MIC_BCLK       GPIO_NUM_9
#define BSP_MIC_WS         GPIO_NUM_10
#define BSP_MIC_DIN        GPIO_NUM_11
#define BSP_SPK_BCLK       GPIO_NUM_9
#define BSP_SPK_WS         GPIO_NUM_10
#define BSP_SPK_DOUT       GPIO_NUM_12
/* #define BSP_SPK_SD         chưa đấu: SD_MODE của MAX98357A để mặc định */

/* Nguồn: breadboard chưa có chip sạc, đo pin, hay công tắc tải
 * #define BSP_CHG_INT / BSP_BAT_ADC (GPIO1–10) / BSP_RAIL_*_EN */
