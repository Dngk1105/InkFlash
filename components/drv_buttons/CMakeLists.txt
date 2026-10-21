idf_component_register(
    SRCS "drv_buttons.c"
    INCLUDE_DIRS "include"
    REQUIRES core device_registry esp_driver_gpio
)
