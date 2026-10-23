set(srcs "drv_display.c")

# Mỗi chip một file. Thêm chip: một khối if ở đây + một mục Kconfig + extern/define trong drv_display.c.
if(CONFIG_INK_DISPLAY_FAKE)
    list(APPEND srcs "display_fake.c")
endif()
if(CONFIG_INK_DISPLAY_UC8253)
    list(APPEND srcs "display_uc8253.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ink_hal core device_registry esp_driver_gpio esp_driver_spi esp_timer
)
