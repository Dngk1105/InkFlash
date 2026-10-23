#include "bsp.h"
#include "bsp_pins.h"
#include "ink_log.h"

typedef struct {
    const char *name;
    int pin;
} pin_entry_t;

/*
 * Bảng chân để kiểm tra trùng lúc khởi động. Chưa có cặp chân nào được phép dùng chung; khi cần
 * (ví dụ hai khối chung một công tắc tải) thì thêm danh sách ngoại lệ vào validate_pins().
 */
static const pin_entry_t s_pins[] = {
#define X(p) { #p, (p) },
    BSP_PIN_DEFAULT_LIST(X)
#undef X
};

static esp_err_t validate_pins(void)
{
    const int n = sizeof(s_pins) / sizeof(s_pins[0]);
    esp_err_t result = ESP_OK;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (s_pins[i].pin != GPIO_NUM_NC && s_pins[i].pin == s_pins[j].pin) {
                INK_LOGE(INK_COMP_SYS, "GPIO%d dung chung: %s va %s", s_pins[i].pin, s_pins[i].name, s_pins[j].name);
                result = ESP_ERR_INVALID_STATE;
            }
        }
    }
    return result;
}

const char *bsp_board_name(void)
{
#if CONFIG_INK_BOARD_BREADBOARD_GD0
    return "breadboard_gd0";
#else
    return "pcb_v1";
#endif
}

esp_err_t bsp_init(void)
{
    esp_err_t err = validate_pins();
    if (err != ESP_OK) {
        return err;
    }
    INK_LOGI(INK_COMP_SYS, "board: %s", bsp_board_name());

    /*
     * Đăng ký driver của board tại đây khi đã viết (GĐ1 trở đi). Mẫu:
     *
     *   const drv_uc8253_cfg_t epd = { .sck = BSP_EPD_SCK, ..., .rotation = BSP_EPD_ROTATION };
     *   ESP_RETURN_ON_ERROR(drv_uc8253_register(&epd), TAG, "epd");
     *
     * Mỗi drv_*_register() tạo một ink_device_t (init/self_test/suspend/resume/recover) và gọi
     * ink_device_register(). Hàm init của driver gắn driver vào HAL (ví dụ ink_display_attach).
     * Nhớ thêm component driver vào REQUIRES trong components/bsp/CMakeLists.txt.
     */
    return ESP_OK;
}
