#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL IMU. Đơn vị mg (1000 mg = 1 g). Hệ trục THIẾT BỊ: x sang phải, y lên trên, z vuông góc màn hình
 * hướng ra ngoài (màn hướng lên trời, nằm yên: z ≈ +1000). Chip gắn xoay thế nào thì khai báo ma trận `mount`
 * khi attach; HAL tự xoay.
 */
typedef struct {
    int16_t x_mg, y_mg, z_mg;
} ink_accel_t;

typedef struct {
    float heading_deg; /* chỉ có nghĩa khi has_heading */
    float roll_deg;
    float pitch_deg;
    bool has_heading;
} ink_orient_t;

typedef struct {
    esp_err_t (*read_accel)(void *ctx, ink_accel_t *out); /* hệ trục của chip */
    /* Chip có fusion trả hướng trực tiếp (đã ở hệ trục thiết bị). NULL: HAL tính roll/pitch từ gia tốc. */
    esp_err_t (*read_orientation)(void *ctx, ink_orient_t *out);
} ink_imu_ops_t;

/* Phía driver. mount[hàng][cột] ∈ {-1, 0, 1}: chip → thiết bị; NULL = ma trận đơn vị. */
esp_err_t ink_imu_attach(const ink_imu_ops_t *ops, void *ctx, const int8_t mount[3][3]);

/* Phía service */
bool ink_imu_present(void);
esp_err_t ink_imu_read_accel(ink_accel_t *out);
esp_err_t ink_imu_read_orientation(ink_orient_t *out);
