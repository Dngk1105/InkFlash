idf_component_register(
    SRCS "ink_hal_imu.c" "ink_hal_display.c" "ink_hal_audio.c" "ink_hal_power.c" "ink_hal_led.c"
    INCLUDE_DIRS "include"
    REQUIRES core freertos
)
