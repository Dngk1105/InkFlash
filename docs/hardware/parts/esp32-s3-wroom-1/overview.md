# ESP32-S3-WROOM-1-N16R8

Vi điều khiển trung tâm (MCU) của toàn bộ dự án InkFlash. Anten PCB tích hợp, Flash ngoài 16 MB (Quad SPI), PSRAM ngoài 8 MB (Octal SPI)

| Thông số                        | Giá trị                    | Ghi chú                                                     |
| --------------------------------- | ---------------------------- | ------------------------------------------------------------ |
| Vi xử lý (CPU)                  | Xtensa 32-bit LX7 lõi kép  | Tần số xung nhịp lên đến 240 MHz, tập lệnh AI Vector |
| Bộ nhớ Flash ngoài             | 16 MB                        | Chuẩn Quad SPI                                              |
| Bộ nhớ PSRAM ngoài             | 8 MB                         | Chuẩn Octal SPI tốc độ cao (120 MHz)                     |
| Bộ nhớ SRAM nội                | 512 KB SRAM + 16 KB RTC SRAM | Bộ nhớ bên trong chip                                     |
| Dải điện áp hoạt động      | 3,0 V – 3,6 V               | Yêu cầu nguồn cấp 3,3 V danh định ổn định           |
| Dòng tiêu thụ đỉnh (RF WiFi) | Lên tới 500 mA             | LDO RT9080 cấp dòng tối đa 600 mA đáp ứng tốt        |
| Dòng ngủ sâu (Deep Sleep)      | ~8 µA                       | Chỉ cấp điện cho RTC Timer và vùng RTC GPIO            |
| Kết nối không dây             | Wi-Fi 802.11 b/g/n + BLE 5.0 | Hỗ trợ Bluetooth Mesh, tầm xa Coded PHY                   |

## Điều hướng tài liệu chi tiết

- **Ràng buộc chân (Pin Constraints & Strapping Pins):** Xem chi tiết tại [pins.md](file:///E:/ESP32_projects/InkFlash/docs/hardware/parts/esp32-s3-wroom-1/pins.md) cùng thư mục.
