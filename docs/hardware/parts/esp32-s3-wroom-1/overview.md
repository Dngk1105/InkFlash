# ESP32-S3-WROOM-1-N16R8

MCU chính. Anten PCB, flash 16 MB, PSRAM octal 8 MB.

| Thông số | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| CPU | Tối đa 240 MHz; firmware chạy 240 MHz | Datasheet module; `sdkconfig.defaults` | Đã chạy trên BB0, 2026-11-03 |
| PSRAM octal | Firmware chạy 80 MHz (ESP-IDF có 40, 80, và 120 MHz thử nghiệm) | `sdkconfig.defaults` | Đã chạy trên BB0, 2026-11-03 |
| RTC memory | RTC fast 8 KB (`0x600FE000`–`0x60100000`), RTC slow 8 KB (`0x50000000`–`0x50002000`); cả hai giữ dữ liệu khi deep sleep. Firmware: `RTC_DATA_ATTR` nằm ở RTC slow (`CONFIG_ESP32S3_RTCDATA_IN_FAST_MEM` tắt), RTC fast làm heap (`CONFIG_ESP_SYSTEM_ALLOW_RTC_FAST_MEM_AS_HEAP`) | `components/soc/esp32s3/include/soc/soc.h` của ESP-IDF v6.1; `sdkconfig` | Đọc từ ESP-IDF, chưa đo, 2026-11-09 |

Ràng buộc chân: `pins.md` cùng thư mục.
