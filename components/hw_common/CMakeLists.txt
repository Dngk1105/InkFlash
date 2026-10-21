idf_component_register(
    SRCS "hw_i2c.c" "hw_i2s.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c esp_driver_i2s esp_driver_gpio
)
