# Thiết kế: docs/design/esp-idf-api.md
idf_component_register(SRCS "gpio_isr.c" "spi_master.c" "i2c_master.c" "i2s_std.c" "adc_oneshot.c"
                            "deep_sleep.c" "nvs.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio esp_driver_spi esp_driver_i2c esp_driver_i2s esp_adc
                       PRIV_REQUIRES nvs_flash esp_hw_support)
