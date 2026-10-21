#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL màn hình. Logic (GUI) không được giả định độ phân giải hay chip điều khiển: hỏi ink_display_get_info().
 *
 * Định dạng framebuffer (cố định cho mọi driver):
 *  - 1 bit mỗi điểm ảnh, bit 1 = trắng, bit 0 = đen.
 *  - Duyệt theo hàng, bit cao (MSB) là điểm bên trái. Mỗi hàng chiếm ceil(width/8) byte.
 *  - Luôn là cả khung hình (full frame). `region` chỉ nói vùng nào cần làm mới.
 * Driver tự đổi sang định dạng riêng của chip (đảo bit, chia hai RAM đen/đỏ, xoay...).
 */
typedef struct {
    uint16_t width, height;
    bool partial_refresh; /* làm mới một phần được không */
    bool deep_clean;      /* có chế độ INK_REFRESH_CLEAN không */
} ink_display_info_t;

typedef struct {
    uint16_t x, y, w, h;
} ink_rect_t;

typedef enum {
    INK_REFRESH_FULL,    /* xoá ghost sạch, chậm, nhấp nháy */
    INK_REFRESH_PARTIAL, /* nhanh; driver không hỗ trợ thì HAL tự dùng FULL */
    INK_REFRESH_CLEAN,   /* đảo đen/trắng toàn màn rồi mới vẽ: sạch hẳn bóng mờ và chấm lốm đốm, rất chậm;
                            driver không hỗ trợ thì HAL tự dùng FULL. Luôn làm cả màn, bỏ qua region. */
} ink_refresh_t;

typedef struct {
    esp_err_t (*get_info)(void *ctx, ink_display_info_t *out);
    esp_err_t (*draw)(void *ctx, const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode);
    esp_err_t (*sleep)(void *ctx); /* đưa panel vào deep sleep, có thể NULL */
} ink_display_ops_t;

esp_err_t ink_display_attach(const ink_display_ops_t *ops, void *ctx);

bool ink_display_present(void);
esp_err_t ink_display_get_info(ink_display_info_t *out);
/* region = NULL: cả màn hình. ESP_ERR_NOT_FOUND nếu board không có màn. */
esp_err_t ink_display_draw(const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode);
esp_err_t ink_display_sleep(void);
