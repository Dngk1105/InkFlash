/* Thiết kế: docs/design/esp-idf-api.md */

/* Dựa theo: examples/peripherals/spi_master/lcd */
#include "ink_periph_examples.h"

#include "esp_check.h"

static const char *TAG = "periph_spi";

esp_err_t ink_periph_examples_spi_init(spi_host_device_t host, gpio_num_t sclk, gpio_num_t mosi, gpio_num_t cs,
                                       int clock_hz, spi_device_handle_t *out_dev)
{
    const spi_bus_config_t bus = {
        .sclk_io_num     = sclk,
        .mosi_io_num     = mosi,
        .miso_io_num     = -1, /* chỉ ghi */
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 16 * 1024,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(host, &bus, SPI_DMA_CH_AUTO), TAG, "spi_bus_initialize");
    const spi_device_interface_config_t dev = {
        .clock_speed_hz = clock_hz,
        .mode           = 0,
        .spics_io_num   = cs,
        .queue_size     = 1,
    };
    return spi_bus_add_device(host, &dev, out_dev);
}

esp_err_t ink_periph_examples_spi_write(spi_device_handle_t dev, const uint8_t *data, size_t len)
{
    /* Dữ liệu gửi bằng DMA phải nằm trong RAM nội có MALLOC_CAP_DMA. */
    spi_transaction_t t = {
        .length    = len * 8, /* đơn vị bit */
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(dev, &t);
}
