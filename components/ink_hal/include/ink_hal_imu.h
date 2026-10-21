#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL IMU: giao diện chuẩn giữa service và driver cảm biến gia tốc.
 *
 * Quy ước (mọi driver phải theo, để logic phía trên không đổi khi thay chip):
 *  - Đơn vị: mg (1000 mg = 1 g).
 *  - Hệ trục của THIẾT BỊ: x sang phải, y lên trên, z vuông góc màn hình và hướng ra ngoài.
 *    Màn hình hướng lên trời, máy nằm yên: z ≈ +1000.
 *  - Chip gắn xoay thế nào thì khai báo bằng ma trận `mount` của board; HAL tự xoay, driver không cần biết.
 *
 * Service chỉ include header này, không include driver hay bsp.
 */
typedef struct {
    int16_t x_mg, y_mg, z_mg;
} ink_accel_t;

/* Hướng của thiết bị, độ. Roll quanh trục y, pitch quanh trục x theo quy ước của chip fusion (BNO055). */
typedef struct {
    float heading_deg; /* 0–360, chỉ có nghĩa khi has_heading */
    float roll_deg;
    float pitch_deg;
    bool has_heading;  /* false nếu chip không có la bàn/fusion */
} ink_orient_t;

typedef struct {
    esp_err_t (*read_accel)(void *ctx, ink_accel_t *out); /* hệ trục của chip, đơn vị mg */
    esp_err_t (*set_rate_hz)(void *ctx, uint16_t hz);     /* có thể NULL */
    /* Chip có fusion (BNO055) trả hướng trực tiếp; đã ở hệ trục thiết bị. NULL thì HAL tự tính từ gia tốc. */
    esp_err_t (*read_orientation)(void *ctx, ink_orient_t *out);
} ink_imu_ops_t;

/* Driver gọi khi init xong. mount[row][col] ∈ {-1,0,1}: chip → thiết bị. */
esp_err_t ink_imu_attach(const ink_imu_ops_t *ops, void *ctx, const int8_t mount[3][3]);

/* Phía service. ESP_ERR_NOT_FOUND nếu board không có IMU hoặc driver chưa sẵn sàng. */
bool ink_imu_present(void);
esp_err_t ink_imu_read_accel(ink_accel_t *out);
esp_err_t ink_imu_set_rate_hz(uint16_t hz);
/* Chip không có fusion: roll/pitch tính từ gia tốc, has_heading = false. */
esp_err_t ink_imu_read_orientation(ink_orient_t *out);
