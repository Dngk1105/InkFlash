idf_component_register(
    SRCS "bsp.c"
    INCLUDE_DIRS "include"
    # Driver người dùng viết được thêm vào REQUIRES khi BSP bắt đầu đăng ký chúng (xem bsp.c).
    REQUIRES core ink_hal esp_driver_gpio
)
