idf_component_register(
    SRCS "ink_device.c" "hal_display.c" "hal_button.c" "hal_audio.c" "hal_imu.c" "hal_power.c" "hal_led.c"
    INCLUDE_DIRS "include"
    REQUIRES core freertos esp_timer
)
