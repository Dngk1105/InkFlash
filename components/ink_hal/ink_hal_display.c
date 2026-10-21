#include "ink_hal_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ink_health.h"

static const ink_display_ops_t *s_ops;
static void *s_ctx;
static ink_display_info_t s_info;
static SemaphoreHandle_t s_lock; /* một lệnh vẽ chạy vài giây; không cho hai task vẽ chồng nhau */

esp_err_t ink_display_attach(const ink_display_ops_t *ops, void *ctx)
{
    if (!ops || !ops->get_info || !ops->draw) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = ops->get_info(ctx, &s_info);
    if (err != ESP_OK) {
        return err;
    }
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
        if (!s_lock) {
            return ESP_ERR_NO_MEM;
        }
    }
    s_ctx = ctx;
    s_ops = ops;
    return ESP_OK;
}

bool ink_display_present(void)
{
    return s_ops != NULL;
}

esp_err_t ink_display_get_info(ink_display_info_t *out)
{
    if (!s_ops) {
        return ESP_ERR_NOT_FOUND;
    }
    *out = s_info;
    return ESP_OK;
}

esp_err_t ink_display_draw(const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode)
{
    if (!s_ops) {
        return ESP_ERR_NOT_FOUND;
    }
    if (!fb) {
        return ESP_ERR_INVALID_ARG;
    }
    if (region && (region->w == 0 || region->h == 0 || region->x + region->w > s_info.width ||
                   region->y + region->h > s_info.height)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (mode == INK_REFRESH_PARTIAL && !s_info.partial_refresh) {
        mode = INK_REFRESH_FULL;
        region = NULL;
    }
    if (mode == INK_REFRESH_CLEAN && !s_info.deep_clean) {
        mode = INK_REFRESH_FULL;
    }
    if (mode != INK_REFRESH_PARTIAL) {
        region = NULL; /* FULL và CLEAN luôn làm cả màn */
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    esp_err_t err = s_ops->draw(s_ctx, fb, region, mode);
    xSemaphoreGive(s_lock);
    ink_health_report(INK_COMP_EPD, err == ESP_OK);
    return err;
}

esp_err_t ink_display_sleep(void)
{
    if (!s_ops) {
        return ESP_ERR_NOT_FOUND;
    }
    if (!s_ops->sleep) {
        return ESP_OK;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    esp_err_t err = s_ops->sleep(s_ctx);
    xSemaphoreGive(s_lock);
    return err;
}
