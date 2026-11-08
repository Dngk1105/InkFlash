/* Design: docs/design/input/input.md */
#pragma once

/*
 * Shell module of gesture: button interrupts, timestamps, timer at gesture deadlines, resync.
 * No task of its own: the ISR and the timer callback only build a message and post it through system;
 * system calls ink_input_handle() on the main task. Only the main task reads or changes this module's state.
 */
#include "esp_err.h"
#include "ink_gesture.h"
#include "ink_hal.h"
#include <stddef.h>
#include <stdint.h>

typedef enum : uint8_t {
    INK_INPUT_MESSAGE_BUTTON_EDGE = 0,
    INK_INPUT_MESSAGE_TIMER_EXPIRED,
} ink_input_message_type_t;

typedef struct {
    ink_input_message_type_t type;

    union {
        ink_gesture_event_t button_edge;
    };
} ink_input_message_t;

/** Post a message to the main queue without waiting. The _from_isr variant must be in IRAM and must not log. */
typedef void (*ink_input_post_t)(const ink_input_message_t *message);
typedef void (*ink_input_post_from_isr_t)(const ink_input_message_t *message);

/**
 * @brief Init gesture, create the timer, enable button interrupts.
 * @par Called from
 * System, once at boot, after the main queue exists.
 * @param buttons Index 0 = A, 1 = B. Must live for the whole program: resync reads the pins later.
 * @retval ESP_ERR_INVALID_ARG count is not 2, or a pointer is NULL.
 * @retval ESP_ERR_NOT_SUPPORTED a button has no interrupt.
 * @retval ESP_ERR_INVALID_STATE already started.
 * @return Other codes come from esp_timer_create() or from enabling the interrupts (gpio_isr_handler_add()).
 */
esp_err_t ink_input_start(const ink_hal_button_device_t *buttons, size_t count, ink_input_post_t post,
                          ink_input_post_from_isr_t post_from_isr);

/**
 * @brief Handle one message of this module.
 * @par Called from
 * Main task.
 * @param out_gestures Gestures just emitted (already logged).
 */
void ink_input_handle(const ink_input_message_t *message, ink_gesture_output_t *out_gestures);

/**
 * @brief Resync the buttons if an edge may have been lost.
 * @par Called from
 * Main task, when the main queue is empty.
 * @param messages_dropped Messages were dropped (main queue full) since the previous call.
 * @param out_gestures Gestures emitted by resync edges.
 */
void ink_input_on_queue_empty(bool messages_dropped, ink_gesture_output_t *out_gestures);

/**
 * @brief Change the gesture config; gestures in progress use it at once.
 * @par Called from
 * Main task (Settings app).
 * @retval ESP_ERR_INVALID_STATE not started.
 * @retval ESP_ERR_INVALID_ARG config is NULL or invalid; the old config stays.
 */
esp_err_t ink_input_set_gesture_config(const ink_gesture_config_t *config);

/**
 * @brief Set the wait mask of one button.
 * @par Called from
 * Main task (system, when an app opens or the scene changes).
 * @param wait_mask INK_GESTURE_WAIT_* bits.
 * @retval ESP_ERR_INVALID_STATE not started.
 * @retval ESP_ERR_INVALID_ARG button is not A or B, or the mask has unknown bits.
 */
esp_err_t ink_input_set_wait_mask(ink_gesture_button_t button, ink_gesture_wait_mask_t wait_mask);
