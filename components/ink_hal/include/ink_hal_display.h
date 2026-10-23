#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/*
 * HAL màn hình. Hợp đồng giữa render service và driver màn.
 *
 * Định dạng framebuffer (cố định cho mọi driver):
 *  - 1 bit/điểm ảnh, bit 1 = trắng, bit 0 = đen; MSB là điểm bên trái; mỗi hàng ceil(width/8) byte.
 *  - Toạ độ LOGIC (đã xoay theo board). Driver tự đổi sang toạ độ và định dạng riêng của chip.
 *  - Luôn là cả khung hình; `region` chỉ nói vùng nào cần làm mới.
 */
typedef enum {
    INK_REFRESH_FULL,    /* làm mới toàn màn, xoá bóng mờ */
    INK_REFRESH_PARTIAL, /* nhanh, không nhấp nháy; driver không hỗ trợ thì HAL đổi thành FULL */
    INK_REFRESH_CLEAN,   /* đảo đen/trắng rồi mới vẽ, sạch hẳn, rất chậm; không hỗ trợ thì HAL đổi thành FULL */
} ink_refresh_t;

typedef struct {
    uint16_t x, y, w, h;
} ink_rect_t;

/* Khả năng của driver; render service dựa vào đây để chọn chiến lược làm mới. */
typedef struct {
    uint16_t width, height;   /* logic, sau khi xoay */
    bool partial_refresh;
    bool deep_clean;
    uint8_t region_align;     /* vùng partial sẽ bị driver mở rộng ra bội số này (điểm ảnh) theo cạnh bị căn */
    uint32_t full_ms_typ;     /* thời gian điển hình, để lập lịch và hiển thị */
    uint32_t partial_ms_typ;
} ink_display_info_t;

typedef struct {
    esp_err_t (*get_info)(void *ctx, ink_display_info_t *out);
    /* Đồng bộ: trả về khi panel đã vẽ xong. Render service gọi từ task của nó. */
    esp_err_t (*draw)(void *ctx, const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode);
    esp_err_t (*sleep)(void *ctx); /* có thể NULL */
} ink_display_ops_t;

/* Phía driver */
esp_err_t ink_display_attach(const ink_display_ops_t *ops, void *ctx);

/* Phía service. ESP_ERR_NOT_FOUND nếu board không có màn. */
bool ink_display_present(void);
esp_err_t ink_display_get_info(ink_display_info_t *out);
esp_err_t ink_display_draw(const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode);
esp_err_t ink_display_sleep(void);
