#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "ink_comp.h"

/*
 * Vòng đời thiết bị phần cứng. Mỗi driver đăng ký một ink_device_t lúc BSP khởi động board.
 * Hàm nào driver không cần thì để NULL.
 */
typedef struct {
    esp_err_t (*init)(void *ctx);      /* cấu hình bus, reset chip, gắn vào HAL (ink_*_attach) */
    esp_err_t (*self_test)(void *ctx); /* đọc chip ID, kiểm tra phản hồi */
    esp_err_t (*suspend)(void *ctx);   /* trước khi ngủ sâu: đưa chip vào chế độ tiết kiệm */
    esp_err_t (*resume)(void *ctx);    /* sau khi thức */
    esp_err_t (*recover)(void *ctx, int level); /* 1 = reset bus/chip, 2 = tắt/bật nguồn khối */
} ink_device_ops_t;

typedef struct {
    const char *name;
    ink_comp_t comp;
    const ink_device_ops_t *ops;
    void *ctx;
    bool optional; /* true: hỏng thì đánh dấu DEGRADED, máy vẫn chạy */
} ink_device_t;

esp_err_t ink_device_register(const ink_device_t *dev);
/*
 * init + self_test từng thiết bị, theo thang phục hồi: thử lại → recover(1) → recover(2) → DEGRADED.
 * Trả ESP_FAIL nếu một thiết bị bắt buộc hỏng.
 */
esp_err_t ink_device_init_all(void);
void ink_device_suspend_all(void); /* thứ tự ngược lúc đăng ký */
void ink_device_resume_all(void);
void ink_device_foreach(void (*cb)(const ink_device_t *dev, void *arg), void *arg);
