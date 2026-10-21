#pragma once

#include "esp_err.h"

/* Console chẩn đoán qua USB Serial/JTAG. Lệnh: status, log. */
esp_err_t diag_console_start(void);
