/* Design: docs/design/tasks.md */
#pragma once

/* Time and one-shot timers. No task of its own: timer callbacks run on the esp_timer task, a helper task, so they
 * may only post messages to the main queue. */
#include "esp_err.h"
#include <stdint.h>

/* ---- Time ---- */

/**
 * @brief Get ms since boot; wraps after 49.7 days (2^32 ms).
 * @par Called from
 * Anywhere, ISR included (in IRAM).
 */
uint32_t ink_platform_now_ms(void);

/* ---- One-shot timer ---- */

typedef struct ink_platform_timer *ink_platform_timer_t;

typedef void (*ink_platform_timer_callback_t)(void *user_data);

/**
 * @brief Create a stopped timer.
 * @par Called from
 * Boot, from the owning module's start function.
 * @param name Must outlive the timer.
 * @retval ESP_ERR_INVALID_ARG callback or out_timer is NULL.
 * @retval ESP_ERR_NO_MEM out of memory.
 */
esp_err_t ink_platform_timer_create(ink_platform_timer_callback_t callback, void *user_data, const char *name,
                                    ink_platform_timer_t *out_timer);

/**
 * @brief Run the callback once after delay_ms; a running timer is restarted.
 * @par Called from
 * Main task.
 * @retval ESP_ERR_INVALID_ARG timer is NULL.
 */
esp_err_t ink_platform_timer_start(ink_platform_timer_t timer, uint32_t delay_ms);

/**
 * @brief Stop the timer; does nothing if it is not running.
 * @par Called from
 * Main task.
 * @retval ESP_ERR_INVALID_ARG timer is NULL.
 */
esp_err_t ink_platform_timer_stop(ink_platform_timer_t timer);
