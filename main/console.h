#pragma once

#include "esp_err.h"

/* Console chẩn đoán qua USB Serial/JTAG: status, heap, tasks, log, intent, app. */
esp_err_t console_start(void);
