idf_component_register(
    SRCS "bsp.c"
    INCLUDE_DIRS "include"
    REQUIRES core device_registry ink_hal
             drv_buttons drv_display drv_imu drv_audio drv_power drv_led
             esp_driver_gpio
)
