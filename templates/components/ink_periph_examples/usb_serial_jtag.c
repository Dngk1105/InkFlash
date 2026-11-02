/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/peripherals/usb_serial_jtag/usb_serial_jtag_echo */
#include "ink_periph_examples.h"

#include "driver/usb_serial_jtag.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdarg.h>
#include <stdio.h>

static const char *TAG = "periph_usb_serial_jtag";

esp_err_t ink_periph_examples_usb_serial_jtag_init(uint32_t rx_buffer_size, uint32_t tx_buffer_size) {
    usb_serial_jtag_driver_config_t config = {
        .rx_buffer_size = rx_buffer_size,
        .tx_buffer_size = tx_buffer_size,
    };
    return usb_serial_jtag_driver_install(&config);
}

size_t ink_periph_examples_usb_serial_jtag_read(uint8_t *buffer, size_t length, uint32_t timeout_ms) {
    const int read = usb_serial_jtag_read_bytes(buffer, length, pdMS_TO_TICKS(timeout_ms));
    return read > 0 ? (size_t)read : 0;
}

esp_err_t ink_periph_examples_usb_serial_jtag_write(const uint8_t *data, size_t length, uint32_t timeout_ms) {
    /* Trả số byte đã chép vào ring buffer TX; ít hơn length là hết thời gian chờ. */
    const int written = usb_serial_jtag_write_bytes(data, length, pdMS_TO_TICKS(timeout_ms));
    ESP_RETURN_ON_FALSE(written == (int)length, ESP_ERR_TIMEOUT, TAG, "write %d/%u", written, (unsigned)length);
    return ESP_OK;
}

/* Log của ESP-IDF đi qua hàm này thay cho in thẳng ra stdout; trả số ký tự như vprintf. */
static int forward_log(const char *format, va_list arguments) {
    char      line[128];
    const int length = vsnprintf(line, sizeof(line), format, arguments);
    if (length > 0) { usb_serial_jtag_write_bytes(line, length < (int)sizeof(line) ? (size_t)length : sizeof(line) - 1, 0); }
    return length;
}

void ink_periph_examples_usb_serial_jtag_take_log(void) {
    esp_log_set_vprintf(forward_log);
}
