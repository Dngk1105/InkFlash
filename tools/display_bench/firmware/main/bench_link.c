/* Thiết kế: docs/design/display/bench.md */

/* Khung gói trên USB-Serial-JTAG. Task đọc tách gói rồi đưa vào hàng đợi task bench; hàm gửi gọi được từ mọi task. */
#include "bench.h"

#include "driver/usb_serial_jtag.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "bench_link";

#define RX_BUFFER_BYTES  8192
#define TX_BUFFER_BYTES  8192
#define READ_TIMEOUT_MS  100
#define WRITE_TIMEOUT_MS 100
#define LOG_LINE_BYTES   200
#define READER_STACK     4096
#define READER_PRIORITY  6

static QueueHandle_t     s_queue;
static SemaphoreHandle_t s_send_lock;

/* ---- CRC ---- */

/* CRC-16/CCITT-FALSE: đa thức 0x1021, khởi đầu 0xFFFF. */
static uint16_t crc16(uint16_t crc, const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)(data[i] << 8);
        for (int bit = 0; bit < 8; bit++) { crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1); }
    }
    return crc;
}

/* ---- Gửi (mọi task) ---- */

static void write_all(const uint8_t *data, size_t length) {
    usb_serial_jtag_write_bytes(data, length, pdMS_TO_TICKS(WRITE_TIMEOUT_MS));
}

void bench_link_send(uint8_t type, uint8_t seq, const uint8_t *payload, size_t length) {
    if (s_send_lock == nullptr || length > BENCH_MAX_PAYLOAD) { return; }
    const uint8_t header[BENCH_HEADER_BYTES] = {BENCH_SYNC_0, BENCH_SYNC_1, type, seq, (uint8_t)length, (uint8_t)(length >> 8)};
    uint16_t      crc                        = crc16(0xFFFF, &header[2], BENCH_HEADER_BYTES - 2);
    crc                                      = crc16(crc, payload, length);
    const uint8_t tail[BENCH_CRC_BYTES]      = {(uint8_t)crc, (uint8_t)(crc >> 8)};

    xSemaphoreTake(s_send_lock, portMAX_DELAY);
    write_all(header, sizeof(header));
    if (length > 0) { write_all(payload, length); }
    write_all(tail, sizeof(tail));
    xSemaphoreGive(s_send_lock);
}

void bench_link_ack(uint8_t seq, esp_err_t error) {
    const int32_t code = error;
    bench_link_send(BENCH_ACK, seq, (const uint8_t *)&code, sizeof(code));
}

/* Log của ESP-IDF thành gói LOG, để không lẫn vào dữ liệu nhị phân. Không log trong hàm này. */
static int forward_log(const char *format, va_list arguments) {
    char line[LOG_LINE_BYTES];
    int  length = vsnprintf(line, sizeof(line), format, arguments);
    if (length <= 0) { return length; }
    size_t size = (size_t)length < sizeof(line) ? (size_t)length : sizeof(line) - 1;
    while (size > 0 && (line[size - 1] == '\n' || line[size - 1] == '\r')) { size--; }
    if (size > 0) { bench_link_send(BENCH_LOG, 0, (const uint8_t *)line, size); }
    return length;
}

/* ---- Đọc (task đọc) ---- */

static size_t read_exact(uint8_t *buffer, size_t length) {
    size_t done = 0;
    while (done < length) {
        const int read = usb_serial_jtag_read_bytes(&buffer[done], length - done, pdMS_TO_TICKS(READ_TIMEOUT_MS));
        if (read <= 0) { return done; }
        done += (size_t)read;
    }
    return done;
}

/* Bỏ byte tới khi gặp A5 5A. */
static void wait_sync(void) {
    uint8_t previous = 0;
    for (;;) {
        uint8_t byte = 0;
        if (usb_serial_jtag_read_bytes(&byte, 1, portMAX_DELAY) != 1) { continue; }
        if (previous == BENCH_SYNC_0 && byte == BENCH_SYNC_1) { return; }
        previous = byte;
    }
}

/* Đọc phần sau sync vào `packet`; trả false nếu thiếu byte, quá dài hoặc sai CRC. */
static bool read_packet(bench_packet_t *packet) {
    uint8_t header[4];
    if (read_exact(header, sizeof(header)) != sizeof(header)) { return false; }
    const uint16_t length = (uint16_t)(header[2] | (header[3] << 8));
    if (length > BENCH_MAX_PAYLOAD) { return false; }
    if (read_exact(packet->payload, length) != length) { return false; }
    uint8_t tail[BENCH_CRC_BYTES];
    if (read_exact(tail, sizeof(tail)) != sizeof(tail)) { return false; }

    uint16_t crc = crc16(0xFFFF, header, sizeof(header));
    crc          = crc16(crc, packet->payload, length);
    if (crc != (uint16_t)(tail[0] | (tail[1] << 8))) { return false; }
    packet->type   = header[0];
    packet->seq    = header[1];
    packet->length = length;
    return true;
}

static void reader_task(void *user_data) {
    (void)user_data;
    static bench_packet_t packet; /* lớn: không để trên stack */
    for (;;) {
        wait_sync();
        if (!read_packet(&packet)) {
            ESP_LOGW(TAG, "gói hỏng, bỏ");
            continue;
        }
        xQueueSend(s_queue, &packet, portMAX_DELAY);
    }
}

/* ---- Khởi động ---- */

esp_err_t bench_link_start(void *packet_queue) {
    s_queue     = packet_queue;
    s_send_lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_send_lock != nullptr, ESP_ERR_NO_MEM, TAG, "mutex");
    usb_serial_jtag_driver_config_t config = {.rx_buffer_size = RX_BUFFER_BYTES, .tx_buffer_size = TX_BUFFER_BYTES};
    ESP_RETURN_ON_ERROR(usb_serial_jtag_driver_install(&config), TAG, "usb_serial_jtag_driver_install");
    esp_log_set_vprintf(forward_log);
    const BaseType_t created = xTaskCreate(reader_task, "bench_link", READER_STACK, nullptr, READER_PRIORITY, nullptr);
    ESP_RETURN_ON_FALSE(created == pdPASS, ESP_ERR_NO_MEM, TAG, "task");
    return ESP_OK;
}
