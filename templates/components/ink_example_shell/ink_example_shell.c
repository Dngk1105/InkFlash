/* Thiết kế: docs/design/code-structure.md */
#include "ink_example_shell.h"

#include <stdbool.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define QUEUE_LEN  8
#define TASK_STACK 3072 /* byte; stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIO  5

static const char *TAG = "ink_example_shell";

static QueueHandle_t s_queue;
static uint32_t s_total; /* trạng thái của khối: chỉ task bên dưới sửa */

static void task_main(void *arg)
{
    (void)arg;
    ink_example_shell_msg_t msg;
    for (;;) {
        if (xQueueReceive(s_queue, &msg, portMAX_DELAY) == pdTRUE) {
            s_total += msg.value;
            ESP_LOGI(TAG, "total %lu", (unsigned long)s_total);
        }
    }
}

esp_err_t ink_example_shell_start(void)
{
    if (s_queue != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_queue = xQueueCreate(QUEUE_LEN, sizeof(ink_example_shell_msg_t));
    if (s_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreatePinnedToCore(task_main, "ink_example", TASK_STACK, NULL, TASK_PRIO, NULL, tskNO_AFFINITY) !=
        pdPASS) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t ink_example_shell_post(const ink_example_shell_msg_t *msg)
{
    if (s_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueSend(s_queue, msg, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

bool IRAM_ATTR ink_example_shell_post_from_isr(const ink_example_shell_msg_t *msg)
{
    if (s_queue == NULL) {
        return false;
    }
    BaseType_t woken = pdFALSE;
    const bool ok = xQueueSendFromISR(s_queue, msg, &woken) == pdTRUE;
    portYIELD_FROM_ISR(woken);
    return ok;
}
