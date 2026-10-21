# Ghi chú về driver

Driver không nằm trong thư mục này. Mỗi loại linh kiện là một component riêng tên `drv_<loại>`:

| Component | Chip thật đã có | Trạng thái |
|---|---|---|
| `drv_buttons` | (GPIO) | Thật, có chống dội |
| `drv_imu` | BNO055 | Đã build; chưa chạy trên máy |
| `drv_display` | UC8253 (GDEY037T03) | Đã build; chưa chạy trên máy; chuỗi lệnh theo GxEPD2 |
| `drv_audio` | INMP441 (mic), MAX98357A (loa) | Đã build; chưa chạy trên máy |
| `drv_led` | WS2812 | Đã build; chưa chạy trên máy |
| `drv_power` | chưa có (BQ25180, TPS22916 chờ `docs/hardware/power.md`) | Chỉ có giả lập |

Mỗi driver theo cùng khuôn: giao diện nằm trong `ink_hal`, mỗi chip một file, chọn chip bằng Kconfig,
chân nhận qua struct cấu hình do BSP điền. Tài nguyên dùng chung (I2C, I2S) lấy từ `hw_common`.
Xem `docs/firmware/architecture.md`, mục "Ranh giới firmware / hardware".
