/* Design: docs/design/code-structure.md */
#pragma once

/*
 * Template of a shell block with a task and a queue. Other blocks post messages to the queue;
 * only the block's own task reads and changes its state. Decisions belong in a pure C block;
 * the shell only receives messages, calls the pure C block, and carries out the result.
 */
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t value;
} ink_example_shell_message_t;

/**
 * @brief Create the queue and the task.
 * @par Called from
 * Main task, once at boot.
 * @retval ESP_ERR_INVALID_STATE already started.
 * @retval ESP_ERR_NO_MEM queue or task could not be created.
 */
esp_err_t ink_example_shell_start(void);

/**
 * @brief Post a message from a task; does not wait.
 * @par Called from
 * Any task.
 * @retval ESP_ERR_INVALID_STATE not started.
 * @retval ESP_ERR_TIMEOUT queue full.
 */
esp_err_t ink_example_shell_post(const ink_example_shell_message_t *message);

/**
 * @brief Post a message from an ISR. In IRAM; no logging, no allocation.
 * @par Called from
 * ISR.
 * @retval true posted.
 * @retval false queue full or not started.
 */
bool ink_example_shell_post_from_isr(const ink_example_shell_message_t *message);
