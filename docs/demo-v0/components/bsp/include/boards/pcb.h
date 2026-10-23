#pragma once

/* PCB InkFlash. Nguồn: docs/hardware/pinmap.md (cột PCB). Xem chú thích ở breadboard_gd0.h. */
#define BSP_BTN_A        GPIO_NUM_0
#define BSP_BTN_ACTIVE_LOW 1

#define BSP_IMU_ADDR     0
#define BSP_IMU_MOUNT    { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }
