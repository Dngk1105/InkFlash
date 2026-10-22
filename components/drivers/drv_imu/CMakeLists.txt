set(srcs "drv_imu.c")

# Mỗi chip một file. Thêm chip mới: thêm một khối if ở đây và một mục trong Kconfig.
if(CONFIG_INK_IMU_FAKE)
    list(APPEND srcs "imu_fake.c")
endif()
if(CONFIG_INK_IMU_BNO055)
    list(APPEND srcs "imu_bno055.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ink_hal core device_registry hw_common esp_driver_gpio esp_driver_i2c
)
