# Thiết kế: docs/design/hardware-variants.md
idf_component_register(SRCS "ink_bsp.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_hal
                       PRIV_REQUIRES ink_drv_gpio_button ink_drv_uc8253 esp_driver_gpio esp_driver_spi)
