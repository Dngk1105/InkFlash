# drv_imu: driver cảm biến gia tốc

Service không biết chip nào đang chạy. Service gọi `ink_imu_read_accel()` hoặc `ink_imu_read_orientation()` của
`components/ink_hal`, đơn vị mg và độ, hệ trục thiết bị.

Chip hiện có: `FAKE` (giả lập), `BNO055` (I2C, có fusion nên trả hướng trực tiếp).

## Thêm một chip mới (ví dụ LSM6DSO), không đụng service

1. Viết `imu_lsm6dso.c` xuất `const drv_imu_chip_t imu_lsm6dso_chip` (khuôn nằm trong `drv_imu_chip.h`):
   `ops.read_accel` trả theo **hệ trục của chip**, đơn vị mg. Việc xoay sang hệ trục thiết bị do HAL làm.
   `ops.read_orientation` để NULL nếu chip không có fusion: HAL tự tính roll/pitch từ gia tốc.
   Hàm `init` dùng `hw_i2c_get_bus()` để lấy bus I2C dùng chung, không tự tạo bus.
2. `Kconfig`: thêm `config INK_IMU_LSM6DSO` vào `choice INK_IMU_DRIVER`.
3. `CMakeLists.txt`: thêm khối `if(CONFIG_INK_IMU_LSM6DSO) list(APPEND srcs "imu_lsm6dso.c")`.
4. `drv_imu.c`: thêm `extern` và `#define IMU_CHIP` tương ứng.
5. Chạy `idf.py menuconfig` → InkFlash IMU driver, chọn chip.

Chân SDA/SCL/INT, tần số I2C, địa chỉ và hướng gắn chip lấy từ `drv_imu_cfg_t`, do BSP điền từ file board.
