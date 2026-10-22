#include <stdio.h>
#include "drv_buttons.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ink_bus.h"
#include "ink_device.h"
#include "ink_log.h"

#define DEBOUNCE_MS 20

static drv_buttons_cfg_t s_cfg;
static TaskHandle_t s_task;
static bool s_pressed[2];
static volatile uint32_t s_isr_count[2]; /* số lần ngắt GPIO nhận được (để chẩn đoán) */
static uint32_t s_event_count[2];        /* số event đã phát lên bus */

static gpio_num_t pin_of(int i)
{
    return i == INK_BTN_A ? s_cfg.pin_a : s_cfg.pin_b;
}

static void IRAM_ATTR button_isr(void *arg)
{
    s_isr_count[(int)(intptr_t)arg]++;
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &woken);
    portYIELD_FROM_ISR(woken);
}

void drv_buttons_dump(void)
{
    for (int i = 0; i < 2; i++) {
        if (pin_of(i) == GPIO_NUM_NC) {
            printf("nut %c: chua noi\n", 'A' + i);
            continue;
        }
        printf("nut %c: GPIO%d muc=%d (%s) ngat=%lu su_kien=%lu\n", 'A' + i, pin_of(i), gpio_get_level(pin_of(i)),
               gpio_get_level(pin_of(i)) == 0 ? "thap" : "cao", (unsigned long)s_isr_count[i],
               (unsigned long)s_event_count[i]);
    }
    printf("kieu dau: %s\n", s_cfg.active_low ? "bam = muc thap (noi GND)" : "bam = muc cao");
}

/* ISR chỉ đánh thức task. Task chờ hết dội rồi đọc mức ổn định và phát event khi trạng thái đổi. */
static void button_task(void *arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        ulTaskNotifyTake(pdTRUE, 0); /* bỏ các lần dội trong lúc chờ */
        for (int i = 0; i < 2; i++) {
            if (pin_of(i) == GPIO_NUM_NC) {
                continue;
            }
            bool pressed = (gpio_get_level(pin_of(i)) == 0) == s_cfg.active_low;
            if (pressed != s_pressed[i]) {
                s_pressed[i] = pressed;
                ink_button_evt_t evt = { .button = i, .action = pressed ? INK_BTN_PRESS : INK_BTN_RELEASE };
                esp_err_t err = ink_bus_post(INK_EVT_BUTTON, &evt, sizeof(evt));
                s_event_count[i]++;
                INK_LOGI(INK_COMP_BUTTONS, INK_LOG_EVT_STATE, "nut %c %s (bus: %s)", 'A' + i,
                         pressed ? "bam" : "nha", esp_err_to_name(err));
            }
        }
    }
}

static esp_err_t buttons_init(void *ctx)
{
    if (!s_task && xTaskCreate(button_task, "drv_buttons", 3072, NULL, 10, &s_task) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) { /* INVALID_STATE = đã cài */
        return err;
    }
    for (int i = 0; i < 2; i++) {
        if (pin_of(i) == GPIO_NUM_NC) {
            continue;
        }
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << pin_of(i),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = s_cfg.active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = s_cfg.active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_ANYEDGE,
        };
        ESP_ERROR_CHECK(gpio_config(&cfg));
        ESP_ERROR_CHECK(gpio_isr_handler_add(pin_of(i), button_isr, (void *)(intptr_t)i));
    }
    return ESP_OK;
}

static const ink_device_ops_t s_ops = { .init = buttons_init };

esp_err_t drv_buttons_register(const drv_buttons_cfg_t *cfg)
{
    s_cfg = *cfg;
    ink_device_t dev = { .name = "buttons", .comp = INK_COMP_BUTTONS, .ops = &s_ops, .optional = false };
    /* TODO: sau khi thức từ deep sleep, lần bấm đánh thức không tính là thao tác (cần biết nguyên nhân thức). */
    return ink_device_register(&dev);
}
