/* Thiết kế: docs/architecture.md */

/* app_main lắp các khối với nhau và vẽ một ảnh thử màn. In bản firmware lúc khởi động
 * (docs/design/hardware-variants.md). */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ink_bsp.h"
#include "ink_system.h"

static const char *TAG = "main";

/* ---- Ảnh thử màn ---- */
/* Ô lớn ở điểm đầu bộ nhớ, ô nhỏ ở cuối hàng đầu: nhìn hai ô biết gốc và chiều x. */

#define TEST_WIDTH     240
#define TEST_HEIGHT    416
#define TEST_ROW_BYTES (TEST_WIDTH / 8)

static uint8_t s_test_frame[TEST_ROW_BYTES * TEST_HEIGHT];

static void fill_black(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    for (uint16_t row = y; row < y + height; row++) {
        for (uint16_t column = x; column < x + width; column++) {
            s_test_frame[row * TEST_ROW_BYTES + column / 8] &= (uint8_t)~(0x80 >> (column % 8));
        }
    }
}

static void draw_test_image(const ink_hal_display_device_t *display)
{
    memset(s_test_frame, 0xFF, sizeof(s_test_frame));
    fill_black(0, 0, 64, 64);
    fill_black(TEST_WIDTH - 24, 0, 24, 24);
    for (uint16_t x = 0; x < TEST_WIDTH; x += 16) { fill_black(x, 160, 8, 80); }          /* sọc dọc */
    for (uint16_t y = 320; y < TEST_HEIGHT; y += 16) { fill_black(0, y, TEST_WIDTH, 8); } /* sọc ngang */

    const ink_hal_display_rect_t whole  = {.width = TEST_WIDTH, .height = TEST_HEIGHT};
    ink_hal_display_timing_t     timing = {};
    ESP_ERROR_CHECK(
        display->operations->draw(display->context, s_test_frame, whole, INK_HAL_DISPLAY_MODE_FULL, &timing));
    ESP_LOGI(TAG, "full refresh: spi %lu ms, panel %lu ms, after %lu ms", (unsigned long)(timing.spi_us / 1000),
             (unsigned long)(timing.panel_us / 1000), (unsigned long)(timing.after_us / 1000));
}

void app_main(void)
{
    const esp_app_desc_t *app_description = esp_app_get_description();
    ESP_LOGI(TAG, "fw %s", app_description->version);

    ESP_ERROR_CHECK(ink_bsp_init());
    const ink_hal_button_device_t *buttons      = nullptr;
    size_t                         button_count = 0;
    ESP_ERROR_CHECK(ink_bsp_buttons(&buttons, &button_count));
    const ink_hal_display_device_t *display = nullptr;
    ESP_ERROR_CHECK(ink_bsp_display(&display));
    draw_test_image(display);
    ESP_ERROR_CHECK(ink_system_start(buttons, button_count));
}
