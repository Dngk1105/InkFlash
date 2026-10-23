#include <string.h>
#include "bsp.h"
#include "drv_audio.h"
#include "drv_buttons.h"
#include "drv_display.h"
#include "drv_imu.h"
#include "drv_led.h"
#include "drv_power.h"
#include "ink_log.h"

typedef struct {
    const char *name;
    int pin;
} pin_entry_t;

static const pin_entry_t s_pins[] = {
#define X(n, p) { #n, (p) },
    BSP_PIN_LIST(X)
#undef X
};

static const char *const s_share[][2] = {
#define X(a, b) { #a, #b },
    BSP_PIN_SHARE_LIST(X)
#undef X
};

static bool may_share(const char *a, const char *b)
{
    for (size_t i = 0; i < sizeof(s_share) / sizeof(s_share[0]); i++) {
        if ((!strcmp(a, s_share[i][0]) && !strcmp(b, s_share[i][1])) ||
            (!strcmp(a, s_share[i][1]) && !strcmp(b, s_share[i][0]))) {
            return true;
        }
    }
    return false;
}

/* Hai chức năng không được dùng chung một chân (trừ cặp trong BSP_PIN_SHARE_LIST). Lỗi cấu hình board nên dừng sớm. */
static esp_err_t validate_pins(void)
{
    const int count = sizeof(s_pins) / sizeof(s_pins[0]);
    esp_err_t result = ESP_OK;
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (s_pins[i].pin != GPIO_NUM_NC && s_pins[i].pin == s_pins[j].pin &&
                !may_share(s_pins[i].name, s_pins[j].name)) {
                INK_LOGE(INK_COMP_SYS, INK_LOG_EVT_SELF_TEST_FAIL, "GPIO%d dung chung: %s va %s",
                         s_pins[i].pin, s_pins[i].name, s_pins[j].name);
                result = ESP_ERR_INVALID_STATE;
            }
        }
    }
    return result;
}

/* Ghi cảnh báo cho các chân chưa khai báo, để biết khối nào đang chạy với driver giả lập/không có chân. */
static void warn_unconnected(void)
{
    char list[256] = "";
    for (size_t i = 0; i < sizeof(s_pins) / sizeof(s_pins[0]); i++) {
        if (s_pins[i].pin == GPIO_NUM_NC && strlen(list) + strlen(s_pins[i].name) + 2 < sizeof(list)) {
            strcat(list, s_pins[i].name);
            strcat(list, " ");
        }
    }
    if (list[0]) {
        INK_LOGW(INK_COMP_SYS, INK_LOG_EVT_STATE, "chan chua noi: %s", list);
    }
}

esp_err_t bsp_register_devices(void)
{
    esp_err_t err = validate_pins();
    if (err != ESP_OK) {
        return err;
    }
    warn_unconnected();

    const drv_buttons_cfg_t buttons = {
        .pin_a = BSP_BTN_A,
        .pin_b = BSP_BTN_B,
        .active_low = BSP_BTN_ACTIVE_LOW,
    };
    const drv_display_cfg_t display = {
        .sck = BSP_EPD_SCK, .mosi = BSP_EPD_MOSI, .cs = BSP_EPD_CS,
        .dc = BSP_EPD_DC, .rst = BSP_EPD_RST, .busy = BSP_EPD_BUSY, .rotation = BSP_EPD_ROTATION,
    };
    const drv_led_cfg_t led = { .pin = BSP_RGB_LED, .brightness = BSP_RGB_BRIGHTNESS };
    const drv_imu_cfg_t imu = {
        .sda = BSP_I2C_SDA, .scl = BSP_I2C_SCL, .int1 = BSP_IMU_INT1,
        .i2c_hz = BSP_I2C_FREQ_HZ, .i2c_addr = BSP_IMU_ADDR, .mount = BSP_IMU_MOUNT,
    };
    const drv_mic_cfg_t mic = { .bclk = BSP_MIC_BCLK, .ws = BSP_MIC_WS, .din = BSP_MIC_DIN };
    const drv_speaker_cfg_t speaker = {
        .bclk = BSP_SPK_BCLK, .ws = BSP_SPK_WS, .dout = BSP_SPK_DOUT, .sd = BSP_SPK_SD,
    };
    /* Chỉ nút nằm trên RTC GPIO mới đánh thức được deep sleep. */
    uint64_t wake_mask = 0;
    const int btn_pins[] = { BSP_BTN_A, BSP_BTN_B };
    const char btn_names[] = { 'A', 'B' };
    for (int i = 0; i < 2; i++) {
        if (btn_pins[i] == GPIO_NUM_NC) {
            continue;
        }
        if (BSP_PIN_IS_RTC(btn_pins[i])) {
            wake_mask |= 1ULL << btn_pins[i];
        } else {
            INK_LOGW(INK_COMP_BUTTONS, INK_LOG_EVT_STATE, "nut %c (GPIO%d) khong phai RTC GPIO: khong danh thuc duoc",
                     btn_names[i], btn_pins[i]);
        }
    }
    const drv_power_cfg_t power = {
        .i2c_sda = BSP_I2C_SDA, .i2c_scl = BSP_I2C_SCL, .i2c_hz = BSP_I2C_FREQ_HZ, .chg_int = BSP_CHG_INT, .bat_adc = BSP_BAT_ADC,
        .rail_en = {
            [INK_RAIL_DISPLAY] = BSP_RAIL_DISPLAY_EN,
            [INK_RAIL_AUDIO] = BSP_RAIL_AUDIO_EN,
            [INK_RAIL_SENSORS] = BSP_RAIL_SENSORS_EN,
        },
        .wake_mask = wake_mask,
    };

    /* Thứ tự: nguồn trước để các khối khác có điện. */
    err = drv_power_register(&power);
    if (err == ESP_OK) {
        err = drv_buttons_register(&buttons);
    }
    if (err == ESP_OK) {
        err = drv_display_register(&display);
    }
    if (err == ESP_OK) {
        err = drv_imu_register(&imu);
    }
    if (err == ESP_OK) {
        err = drv_mic_register(&mic);
    }
    if (err == ESP_OK) {
        err = drv_speaker_register(&speaker);
    }
    if (err == ESP_OK) {
        err = drv_led_register(&led);
    }
    /* TODO: sd — khi docs/hardware có thông số. */
    return err;
}
