/* Design: docs/design/tasks.md */
#pragma once

/* Time, one-shot timers, RTC slots and deep sleep. No task of its own: timer callbacks run on the esp_timer task,
 * a helper task, so they may only post messages to the main queue. */
#include "esp_err.h"
#include <stddef.h>
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

/* ---- RTC slots ---- */
/* Design: docs/design/wake-session.md. RTC slow memory keeps these bytes across deep sleep; power loss clears them. */

/** One fixed-size RTC slot per module that keeps a snapshot. */
typedef enum : uint8_t {
    INK_PLATFORM_RTC_SLOT_SYSTEM = 0, /**< background frame metadata, open app index */
    INK_PLATFORM_RTC_SLOT_APP,        /**< open app: scene stack and app-level state (its save()) */
    INK_PLATFORM_RTC_SLOT_COUNT,
} ink_platform_rtc_slot_t;

/**
 * @brief Get the bytes of one RTC slot (header included); 4-byte aligned.
 * @par Called from
 * Main task.
 * @retval ESP_ERR_INVALID_ARG slot is out of range or a pointer is NULL.
 */
esp_err_t ink_platform_rtc_slot(ink_platform_rtc_slot_t slot, uint8_t **out_bytes, size_t *out_size);

/**
 * @brief CRC32 (little endian, ROM) of `length` bytes; matches ink_session_crc_t.
 * @par Called from
 * Main task.
 */
uint32_t ink_platform_crc32(const uint8_t *data, size_t length);

/* ---- Reset reason and deep sleep ---- */

/**
 * @brief Whether this boot is a deep sleep wake; every other reset reason is a cold boot.
 * @par Called from
 * Main task, at boot.
 */
bool ink_platform_woke_from_deep_sleep(void);

/**
 * @brief Wait until every pin of `wake_pin_mask` is released (high), then enter deep sleep; any of them pulled low
 *        wakes the chip. Does not return.
 * @par Called from
 * Main task (stack in internal RAM), after display reported power off.
 * @param wake_pin_mask Bit n = GPIOn; every pin must be an RTC GPIO, buttons wired to GND.
 */
[[noreturn]] void ink_platform_deep_sleep(uint64_t wake_pin_mask);
