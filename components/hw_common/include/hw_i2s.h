#pragma once

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"

/*
 * I2S dùng chung cho mic và loa. Hai khối chung chân BCLK/WS nên phải chạy trên cùng một cổng I2S,
 * chế độ song công (duplex): mic nhận (RX), loa phát (TX), cùng một xung nhịp.
 *
 * Quy trình: mỗi driver gọi hw_i2s_request_* lúc đăng ký, rồi hw_i2s_get_rx/tx lúc init.
 * Kênh được tạo một lần, với đủ các chiều đã đăng ký, ở lần get đầu tiên.
 * Cả hai chiều dùng khung 32 bit, stereo, tốc độ 16 kHz (BCLK = 64 × 16 kHz).
 */
void hw_i2s_request_rx(gpio_num_t bclk, gpio_num_t ws, gpio_num_t din);
void hw_i2s_request_tx(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout);

esp_err_t hw_i2s_get_rx(i2s_chan_handle_t *out);
esp_err_t hw_i2s_get_tx(i2s_chan_handle_t *out);
