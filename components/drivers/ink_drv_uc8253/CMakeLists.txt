# Thiết kế: docs/design/display-pipeline.md
idf_component_register(SRCS "ink_drv_uc8253.c"
                       INCLUDE_DIRS "include"
                       REQUIRES ink_hal esp_driver_gpio esp_driver_spi freertos
                       PRIV_REQUIRES esp_timer)
