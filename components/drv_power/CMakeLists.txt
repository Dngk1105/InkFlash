set(srcs "drv_power.c")

if(CONFIG_INK_POWER_FAKE)
    list(APPEND srcs "power_fake.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ink_hal core device_registry esp_driver_gpio
)
