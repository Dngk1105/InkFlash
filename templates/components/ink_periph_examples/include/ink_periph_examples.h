/* Thiết kế: docs/design/esp-idf-api.md */
#pragma once

/*
 * Mẫu dùng đúng API ESP-IDF v6.1 cho từng ngoại vi. Mỗi ngoại vi một file .c, ghi rõ ví dụ chính thức nó dựa theo.
 * Chân luôn truyền qua tham số (BSP quyết định chân), không viết cứng.
 * Các hàm ở đây chỉ để build kiểm tra và để chép theo, firmware không gọi.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

/* GPIO đầu vào có ngắt hai cạnh; `isr` chạy trong ISR nên phải có IRAM_ATTR. */
esp_err_t ink_periph_examples_gpio_isr_init(gpio_num_t pin, gpio_isr_t isr, void *arg);

/* SPI master chỉ ghi (như màn e-paper): khởi tạo bus và thêm một thiết bị. */
esp_err_t ink_periph_examples_spi_init(spi_host_device_t host, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t cs,
                                       int clock_hz, spi_device_handle_t *out_dev);
/* Gửi một khối dữ liệu, chờ xong. */
esp_err_t ink_periph_examples_spi_write(spi_device_handle_t dev, const uint8_t *data, size_t len);

/* I2C master (driver mới, không dùng driver/i2c.h): tạo bus và thêm một thiết bị 7 bit. */
esp_err_t ink_periph_examples_i2c_init(gpio_num_t sda, gpio_num_t scl, uint16_t addr, uint32_t hz,
                                       i2c_master_bus_handle_t *out_bus, i2c_master_dev_handle_t *out_dev);
/* Ghi địa chỉ thanh ghi rồi đọc `len` byte. */
esp_err_t ink_periph_examples_i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *out, size_t len);

/* I2S chuẩn Philips, kênh TX cho loa. */
esp_err_t ink_periph_examples_i2s_tx_init(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, uint32_t sample_rate,
                                          i2s_chan_handle_t *out_tx);

/* ADC oneshot có hiệu chỉnh, đọc điện áp (mV) trên một kênh của ADC1. */
esp_err_t ink_periph_examples_adc_read_mv(adc_channel_t channel, int *out_mv);

/* Cấu hình nút đánh thức (mức thấp) rồi ngủ sâu. Không quay về. */
void ink_periph_examples_deep_sleep(uint64_t wake_pin_mask);
/* true nếu lần khởi động này là thức dậy từ ngủ sâu bằng nút (EXT1). */
bool ink_periph_examples_woke_by_button(void);
/* Bộ đếm nằm trong RTC memory, còn giữ qua ngủ sâu. */
uint32_t ink_periph_examples_rtc_counter_next(void);

/* NVS: đọc số u32 theo khoá, không có thì trả `def`; ghi số u32 rồi commit. */
esp_err_t ink_periph_examples_nvs_get_u32(const char *ns, const char *key, uint32_t def, uint32_t *out);
esp_err_t ink_periph_examples_nvs_set_u32(const char *ns, const char *key, uint32_t value);
