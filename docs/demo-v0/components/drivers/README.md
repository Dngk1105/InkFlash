# Drivers

Thư mục này chứa toàn bộ driver linh kiện phần cứng (`drv_*`). Mỗi loại linh kiện là một component riêng:

| Component | Thư mục | Chip thật đã có | Trạng thái |
|---|---|---|---|
| `drv_buttons` | `components/drivers/drv_buttons` | (GPIO) | Thật, có chống dội |
| `drv_imu` | `components/drivers/drv_imu` | BNO055 | Đã build; chưa chạy trên máy |
| `drv_display` | `components/drivers/drv_display` | UC8253 (GDEY037T03) | Đã build; chưa chạy trên máy; chuỗi lệnh theo GxEPD2 |
| `drv_audio` | `components/drivers/drv_audio` | INMP441 (mic), MAX98357A (loa) | Đã build; chưa chạy trên máy |
| `drv_led` | `components/drivers/drv_led` | WS2812 | Đã build; chưa chạy trên máy |
| `drv_power` | `components/drivers/drv_power` | ESP32-S3 sleep (chờ BQ25180) | Thật (deep sleep RTC), giả lập |

## Quy chuẩn driver

Mỗi driver tuân theo cùng khuôn mẫu:
- Giao diện chuẩn nằm trong `components/ink_hal`.
- Chân và cấu hình do `components/bsp` truyền qua struct.
- Tài nguyên bus dùng chung (I2C, I2S) lấy từ `components/hw_common`.
- Được khai báo vào CMake gốc qua `EXTRA_COMPONENT_DIRS "components/drivers"`.
- Chi tiết kiến trúc: xem `docs/firmware/architecture.md`, mục "Ranh giới firmware / hardware".
