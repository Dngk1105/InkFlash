set(srcs "drv_led.c")

if(CONFIG_INK_LED_WS2812)
    list(APPEND srcs "led_ws2812.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ink_hal core device_registry esp_driver_rmt esp_driver_gpio
)
