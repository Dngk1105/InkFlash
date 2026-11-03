# ESP32-S3-WROOM-1-N16R8

MCU chính. Anten PCB, flash 16 MB, PSRAM octal 8 MB.

| Thông số | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| CPU | Tối đa 240 MHz; firmware chạy 240 MHz | Datasheet module; `sdkconfig.defaults` | Đã chạy trên BB0, 2026-11-03 |
| PSRAM octal | Firmware chạy 80 MHz (ESP-IDF có 40, 80, và 120 MHz thử nghiệm) | `sdkconfig.defaults` | Đã chạy trên BB0, 2026-11-03 |

Ràng buộc chân: `pins.md` cùng thư mục.
