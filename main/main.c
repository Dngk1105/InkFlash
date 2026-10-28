/* Thiết kế: docs/architecture.md */

/* app_main tạm cho Mốc 1 bước 2: đo partial refresh của ink_drv_uc8253
 * (docs/design/display-pipeline.md). Không chạy ink_system: bench giữ callback
 * nút và console. In bản firmware lúc khởi động
 * (docs/design/hardware-variants.md). */
#include "esp_app_desc.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_bsp.h"
#include "ink_drv_uc8253.h"
#include "ink_platform.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "main";

/* ---- Khung ảnh và vùng đo ---- */

#define FRAME_WIDTH INK_DRV_UC8253_WIDTH
#define FRAME_HEIGHT INK_DRV_UC8253_HEIGHT
#define FRAME_ROW_BYTES (FRAME_WIDTH / 8)
#define PATTERN_BITS 10 /* số cột của mã nhị phân trong vùng */

static uint8_t s_frame[INK_DRV_UC8253_FRAME_BYTES];

typedef enum : uint8_t {
  REGION_SMALL = 0, /* một khung chữ */
  REGION_LARGE,     /* cả màn, vẽ kiểu partial */
} region_t;

static const ink_hal_display_rect_t s_regions[] = {
    [REGION_SMALL] = {.x = 80, .y = 40, .width = 80, .height = 40},
    [REGION_LARGE] = {.x = 0,
                      .y = 0,
                      .width = FRAME_WIDTH,
                      .height = FRAME_HEIGHT},
};

static void fill(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 bool black) {
  for (uint16_t row = y; row < y + height; row++) {
    for (uint16_t column = x; column < x + width; column++) {
      uint8_t *byte = &s_frame[row * FRAME_ROW_BYTES + column / 8];
      const uint8_t bit = 0x80 >> (column % 8);
      *byte = black ? (uint8_t)(*byte & ~bit) : (uint8_t)(*byte | bit);
    }
  }
}

/* Vẽ số đếm thành PATTERN_BITS cột đen/trắng: mỗi lần nội dung khác, cột đã đổi
 * để lại bóng mờ thì dễ thấy. */
static void draw_pattern(region_t region, uint32_t count) {
  const ink_hal_display_rect_t *rect = &s_regions[region];
  const uint16_t column_width = rect->width / PATTERN_BITS;
  fill(rect->x, rect->y, rect->width, rect->height, false);
  for (uint16_t bit = 0; bit < PATTERN_BITS; bit++) {
    if (count & (1u << bit)) {
      fill(rect->x + bit * column_width, rect->y, column_width, rect->height,
           true);
    }
  }
}

/* ---- Message vào task bench ---- */
/* ISR nút và console chỉ đẩy message; task bench vẽ. */

typedef enum : uint8_t {
  MESSAGE_PRESS = 0,
  MESSAGE_BENCH,
  MESSAGE_FULL,
} message_type_t;

typedef struct {
  message_type_t type;
  region_t region;
  int64_t edge_us; /* MESSAGE_PRESS: lúc ISR nút chạy */
  uint16_t count;  /* MESSAGE_BENCH: số lần vẽ */
  ink_drv_uc8253_partial_options_t options;
} message_t;

static QueueHandle_t s_queue;

/* user_data là vùng: nút A vùng nhỏ, nút B vùng lớn. */
static void IRAM_ATTR on_button_edge(void *user_data, bool pressed) {
  if (!pressed) {
    return;
  }
  const message_t message = {
      .type = MESSAGE_PRESS,
      .region = (region_t)(uintptr_t)user_data,
      .edge_us = esp_timer_get_time(),
  };
  BaseType_t higher_priority_task_woken = pdFALSE;
  xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

/* ---- Lệnh console ---- */

static int handle_bench_command(int argc, char **argv) {
  message_t message = {.type = MESSAGE_BENCH};
  const bool valid =
      (argc == 5 || argc == 6) &&
      (strcmp(argv[1], "small") == 0 || strcmp(argv[1], "large") == 0);
  if (!valid) {
    printf("bench <small|large> <scan 0|1> <temp: 0 = sensor, khác 0 = ép °C> "
           "<n> [keep 0|1]\n");
    return 1;
  }
  message.options.keep_power_on = argc == 6 && atoi(argv[5]) != 0;
  message.region = strcmp(argv[1], "small") == 0 ? REGION_SMALL : REGION_LARGE;
  message.options.pt_scan = atoi(argv[2]) != 0;
  message.options.temperature = (uint8_t)atoi(argv[3]);
  message.options.force_temperature = message.options.temperature != 0;
  message.count = (uint16_t)atoi(argv[4]);
  xQueueSend(s_queue, &message, 0);
  return 0;
}

static int handle_full_command(int argc, char **argv) {
  (void)argc;
  (void)argv;
  const message_t message = {.type = MESSAGE_FULL};
  xQueueSend(s_queue, &message, 0);
  return 0;
}

/* ---- Task bench ---- */

typedef struct {
  uint32_t count;
  uint64_t sum_us[4];
  uint32_t min_us[4];
  uint32_t max_us[4];
} stats_t;

enum { STAT_TOTAL = 0, STAT_SPI, STAT_PANEL, STAT_AFTER };

static const ink_hal_display_device_t *s_display;
static ink_drv_uc8253_t *s_driver;
static ink_drv_uc8253_partial_options_t s_options = {
    .pt_scan = true, .force_temperature = true, .temperature = 110};
static uint32_t s_draw_count; /* số lần partial từ lần full cuối */

static void stats_add(stats_t *stats, const uint32_t values_us[4]) {
  for (size_t i = 0; i < 4; i++) {
    stats->sum_us[i] += values_us[i];
    if (stats->count == 0 || values_us[i] < stats->min_us[i]) {
      stats->min_us[i] = values_us[i];
    }
    if (values_us[i] > stats->max_us[i]) {
      stats->max_us[i] = values_us[i];
    }
  }
  stats->count++;
}

/* Vẽ partial, trả bốn số: edge → BUSY lên cao, spi, panel, after. */
static esp_err_t draw_partial(region_t region, int64_t edge_us,
                              uint32_t out_values_us[4]) {
  draw_pattern(region, ++s_draw_count);
  ink_hal_display_timing_t timing = {};
  const int64_t start_us = esp_timer_get_time();
  ESP_RETURN_ON_ERROR(s_display->operations->draw(
                          s_display->context, s_frame, s_regions[region],
                          INK_HAL_DISPLAY_MODE_PARTIAL, &timing),
                      TAG, "draw");
  out_values_us[STAT_TOTAL] =
      (uint32_t)(start_us - edge_us) + timing.spi_us + timing.panel_us;
  out_values_us[STAT_SPI] = timing.spi_us;
  out_values_us[STAT_PANEL] = timing.panel_us;
  out_values_us[STAT_AFTER] = timing.after_us;
  return ESP_OK;
}

static void log_draw(region_t region, const uint32_t values_us[4]) {
  const ink_hal_display_rect_t *rect = &s_regions[region];
  ESP_LOGI(TAG,
           "partial #%lu %ux%u scan=%u temp=%u keep=%u: edge→busy %.1f ms (spi "
           "%.1f, panel %.1f, after %.1f)",
           (unsigned long)s_draw_count, rect->width, rect->height,
           s_options.pt_scan,
           s_options.force_temperature ? s_options.temperature : 0,
           s_options.keep_power_on, values_us[STAT_TOTAL] / 1000.0,
           values_us[STAT_SPI] / 1000.0, values_us[STAT_PANEL] / 1000.0,
           values_us[STAT_AFTER] / 1000.0);
}

static void run_bench(const message_t *message) {
  s_options = message->options;
  ink_drv_uc8253_set_partial_options(s_driver, &s_options);
  stats_t stats = {};
  for (uint16_t i = 0; i < message->count; i++) {
    uint32_t values_us[4];
    if (draw_partial(message->region, esp_timer_get_time(), values_us) !=
        ESP_OK) {
      return;
    }
    log_draw(message->region, values_us);
    stats_add(&stats, values_us);
  }
  if (stats.count == 0) {
    return;
  }
  static const char *const names[] = {"edge→busy", "spi", "panel", "after"};
  for (size_t i = 0; i < 4; i++) {
    ESP_LOGI(TAG, "bench %s: avg %.1f min %.1f max %.1f ms (n=%lu)", names[i],
             stats.sum_us[i] / 1000.0 / stats.count, stats.min_us[i] / 1000.0,
             stats.max_us[i] / 1000.0, (unsigned long)stats.count);
  }
}

static void draw_full(void) {
  memset(s_frame, 0xFF, sizeof(s_frame));
  for (size_t i = 0; i < sizeof(s_regions) / sizeof(s_regions[0]); i++) {
    draw_pattern((region_t)i, 0);
  }
  const ink_hal_display_rect_t whole = {.width = FRAME_WIDTH,
                                        .height = FRAME_HEIGHT};
  ink_hal_display_timing_t timing = {};
  ESP_ERROR_CHECK(s_display->operations->draw(
      s_display->context, s_frame, whole, INK_HAL_DISPLAY_MODE_FULL, &timing));
  s_draw_count = 0;
  ESP_LOGI(TAG, "full refresh: spi %lu ms, panel %lu ms, after %lu ms",
           (unsigned long)(timing.spi_us / 1000),
           (unsigned long)(timing.panel_us / 1000),
           (unsigned long)(timing.after_us / 1000));
}

static void task_bench(void *user_data) {
  (void)user_data;
  draw_full();
  message_t message;
  for (;;) {
    if (xQueueReceive(s_queue, &message, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    switch (message.type) {
    case MESSAGE_PRESS: {
      uint32_t values_us[4];
      if (draw_partial(message.region, message.edge_us, values_us) == ESP_OK) {
        log_draw(message.region, values_us);
      }
      break;
    }
    case MESSAGE_BENCH:
      run_bench(&message);
      break;
    case MESSAGE_FULL:
      draw_full();
      break;
    }
    /* nút rung trong lúc vẽ: bỏ các lần bấm dồn lại */
    while (xQueuePeek(s_queue, &message, 0) == pdTRUE &&
           message.type == MESSAGE_PRESS) {
      xQueueReceive(s_queue, &message, 0);
    }
  }
}

void app_main(void) {
  const esp_app_desc_t *app_description = esp_app_get_description();
  ESP_LOGI(TAG, "fw %s", app_description->version);

  ESP_ERROR_CHECK(ink_bsp_init());
  const ink_hal_button_device_t *buttons = nullptr;
  size_t button_count = 0;
  ESP_ERROR_CHECK(ink_bsp_buttons(&buttons, &button_count));
  ESP_ERROR_CHECK(ink_bsp_display(&s_display));
  s_driver = s_display->context;

  s_queue = xQueueCreate(8, sizeof(message_t));
  ESP_ERROR_CHECK(s_queue ? ESP_OK : ESP_ERR_NO_MEM);
  ESP_ERROR_CHECK(xTaskCreate(task_bench, "bench", 4096, nullptr, 5, nullptr) ==
                          pdPASS
                      ? ESP_OK
                      : ESP_ERR_NO_MEM);

  for (size_t i = 0; i < button_count && i < 2; i++) {
    ESP_ERROR_CHECK(buttons[i].operations->set_callback(
        buttons[i].context, on_button_edge, (void *)(uintptr_t)i));
  }
  ESP_ERROR_CHECK(ink_platform_console_init("ink> "));
  ESP_ERROR_CHECK(ink_platform_console_add(
      "bench", "Đo partial refresh",
      "<small|large> <scan 0|1> <temp> <n> [keep 0|1]", handle_bench_command));
  ESP_ERROR_CHECK(ink_platform_console_add("full", "Full refresh, về ảnh gốc",
                                           nullptr, handle_full_command));
  ESP_ERROR_CHECK(ink_platform_console_start());
}
