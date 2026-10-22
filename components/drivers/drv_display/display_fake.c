#include "drv_display_chip.h"
#include "ink_log.h"

/* Kích thước GIẢ LẬP để chạy thử logic, không phải thông số của module WeAct. */
#define FAKE_W 400
#define FAKE_H 240

static uint32_t s_draws;

static esp_err_t fake_info(void *ctx, ink_display_info_t *out)
{
    *out = (ink_display_info_t){ .width = FAKE_W, .height = FAKE_H, .partial_refresh = true, .deep_clean = true };
    return ESP_OK;
}

static esp_err_t fake_draw(void *ctx, const uint8_t *fb, const ink_rect_t *region, ink_refresh_t mode)
{
    INK_LOGI(INK_COMP_EPD, INK_LOG_EVT_STATE, "fake draw #%lu %s %ux%u", (unsigned long)++s_draws,
             mode == INK_REFRESH_FULL ? "full" : mode == INK_REFRESH_CLEAN ? "clean" : "partial",
             region ? region->w : FAKE_W,
             region ? region->h : FAKE_H);
    return ESP_OK;
}

static esp_err_t fake_sleep(void *ctx)
{
    INK_LOGI(INK_COMP_EPD, INK_LOG_EVT_STATE, "fake sleep");
    return ESP_OK;
}

static const ink_display_ops_t s_ops = {
    .get_info = fake_info,
    .draw = fake_draw,
    .sleep = fake_sleep,
};

const drv_display_chip_t display_fake_chip = {
    .ops = &s_ops,
};
