/* Thiết kế: docs/design/code-structure.md */
#include "ink_example_shell.h"

#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdbool.h>

static const char *TAG = "ink_example_shell";

/* ---- Hàng đợi ---- */

#define QUEUE_LENGTH 8

static QueueHandle_t s_queue;

esp_err_t ink_example_shell_post(const ink_example_shell_message_t *message) {
  if (s_queue == nullptr) {
    return ESP_ERR_INVALID_STATE;
  }
  return xQueueSend(s_queue, message, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

bool IRAM_ATTR
ink_example_shell_post_from_isr(const ink_example_shell_message_t *message) {
  if (s_queue == nullptr) {
    return false;
  }
  BaseType_t higher_priority_task_woken = pdFALSE;
  const bool sent = xQueueSendFromISR(s_queue, message,
                                      &higher_priority_task_woken) == pdTRUE;
  portYIELD_FROM_ISR(higher_priority_task_woken);
  return sent;
}

/* ---- Task ---- */

#define TASK_STACK_BYTES                                                       \
  3072 /* stack nằm trong RAM nội (mặc định của xTaskCreatePinnedToCore) */
#define TASK_PRIORITY 5

static uint32_t s_total; /* trạng thái của khối: chỉ task bên dưới sửa */

static void task_main(void *user_data) {
  (void)user_data;
  ink_example_shell_message_t message;
  for (;;) {
    if (xQueueReceive(s_queue, &message, portMAX_DELAY) == pdTRUE) {
      s_total += message.value;
      ESP_LOGI(TAG, "total %lu", (unsigned long)s_total);
    }
  }
}

/* ---- Khởi động ---- */

esp_err_t ink_example_shell_start(void) {
  if (s_queue != nullptr) {
    return ESP_ERR_INVALID_STATE;
  }
  s_queue = xQueueCreate(QUEUE_LENGTH, sizeof(ink_example_shell_message_t));
  if (s_queue == nullptr) {
    return ESP_ERR_NO_MEM;
  }
  if (xTaskCreatePinnedToCore(task_main, "ink_example", TASK_STACK_BYTES,
                              nullptr, TASK_PRIORITY, nullptr,
                              tskNO_AFFINITY) != pdPASS) {
    vQueueDelete(s_queue);
    s_queue = nullptr;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}
