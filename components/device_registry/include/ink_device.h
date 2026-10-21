#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "ink_comp.h"

/*
 * Device registry: mỗi thiết bị phần cứng đăng ký bảng hàm dưới đây.
 * Hàm nào không cần thì để NULL.
 */
typedef struct {
    esp_err_t (*init)(void *ctx);
    esp_err_t (*self_test)(void *ctx);
    esp_err_t (*suspend)(void *ctx);
    esp_err_t (*resume)(void *ctx);
    /* level 1 = reset bus, level 2 = tắt/bật nguồn khối. */
    esp_err_t (*recover)(void *ctx, int level);
} ink_device_ops_t;

typedef struct {
    const char *name;
    ink_comp_t comp;
    const ink_device_ops_t *ops;
    void *ctx;
    bool optional; /* true: lỗi thì DEGRADED, máy vẫn chạy */
} ink_device_t;

esp_err_t ink_device_register(const ink_device_t *dev);
/* init + self_test cho mọi thiết bị, có thang phục hồi. Trả ESP_FAIL nếu thiết bị bắt buộc hỏng. */
esp_err_t ink_device_init_all(void);
void ink_device_suspend_all(void);
void ink_device_resume_all(void);
void ink_device_foreach(void (*cb)(const ink_device_t *dev, void *arg), void *arg);
