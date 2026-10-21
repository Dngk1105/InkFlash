idf_component_register(
    SRCS "inkflash_main.c" "diag_console.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES core device_registry ink_hal drv_imu drv_buttons bsp services remote app_update console esp_psram spi_flash esp_driver_gpio
)
