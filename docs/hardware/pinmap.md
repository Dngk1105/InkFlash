# Pinmap GPIO

File này là nguồn cho `components/bsp/include/boards/*.h`. Đổi chân thì sửa ở đây trước, rồi mới sửa code.

## Ràng buộc của ESP32-S3-WROOM-1-N16R8

- Không dùng GPIO22–25 (không tồn tại), GPIO26–32 (flash), GPIO33–37 (PSRAM octal). `bsp_pins.h` chặn lúc biên dịch.
- GPIO0, GPIO3, GPIO45, GPIO46 là chân strapping (đọc lúc khởi động). Dùng được nhưng phải chú ý mức kéo lên/xuống.
- GPIO19/20 là USB D-/D+ (console USB Serial/JTAG).
- Chân đánh thức deep sleep phải là RTC GPIO (GPIO0–21).
- Đo pin phải dùng ADC1 (GPIO1–10), vì ADC2 (GPIO11–20) tranh với WiFi.

## Bảng chân

Quy ước cột "Trạng thái" theo [../README.md](../README.md). Nguồn của cột Breadboard: bảng đấu dây ngày 2026-10-21.

| Chức năng | Breadboard GĐ0 | PCB | Ghi chú | Trạng thái |
|---|---|---|---|---|
| Nút A (wake) | GPIO8 (BTN1) | GPIO0 | PCB dự kiến GPIO0 (kiêm download mode). GPIO8 là RTC: đánh thức được | Breadboard: giả định nối GND, chưa xác nhận |
| Nút B (wake) | GPIO46 (BTN2) | | GPIO46 KHÔNG phải RTC: không đánh thức được. Xem mục "GPIO46" bên dưới | Breadboard: giả định nối GND, chưa xác nhận |
| EPD SCK (module ghi SCL) | GPIO4 | | | Đã đấu |
| EPD MOSI (module ghi SDA) | GPIO5 | | | Đã đấu |
| EPD CS | GPIO6 | | | Đã đấu |
| EPD DC | GPIO7 | | | Đã đấu |
| EPD RST (module ghi RES) | GPIO15 | | | Đã đấu |
| EPD BUSY | GPIO16 | | | Đã đấu |
| I2C SDA | GPIO17 | | Hiện chỉ BNO055 | Đã đấu |
| I2C SCL | GPIO18 | | Tần số 100 kHz | Đã đấu |
| IMU INT | chưa đấu | | | |
| I2S BCLK (mic + loa chung) | GPIO9 | | Mic SCK và loa BCLK | Đã đấu |
| I2S WS (mic + loa chung) | GPIO10 | | Mic WS và loa LRC | Đã đấu |
| I2S mic DIN | GPIO11 | | INMP441 SD | Đã đấu |
| I2S loa DOUT | GPIO12 | | MAX98357A DIN | Đã đấu |
| Amp SD (bật/tắt) | chưa đấu | | | |
| TPS22916 EN (nguồn khối phụ) | chưa có | | | |
| BQ25180 INT | chưa có | | | |
| Đo pin (ADC) | chưa có | | Phải là GPIO1–10 | |
| microSD CLK/CMD/D0 | chưa có | | Tuỳ chọn | |
| LED RGB (WS2812 trên DevKit) | GPIO48 | | Chưa có driver, chỉ giữ chỗ | Đã đấu sẵn trên DevKit |
| EPD MISO | không dùng | | Màn chỉ ghi | |
| GPIO13, GPIO14 | trống | | Còn dùng được | |

## GPIO46 (nút B trên breadboard)

- **Không đánh thức được deep sleep**, vì chỉ GPIO0–21 là RTC GPIO. Trên breadboard chỉ nút A đánh thức máy; nút B chỉ dùng khi máy đang thức. Trên PCB cả hai nút phải là RTC GPIO (build sẽ báo lỗi nếu không).
- **Là chân strapping** (nguồn: [esptool, Boot Mode Selection](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html)): mặc định có pull-down yếu lúc reset. Kết hợp với GPIO0 chọn chế độ nạp code: GPIO0 = 0 và GPIO46 = 0 thì vào download mode. GPIO46 cũng liên quan tới in ROM log.
- **Rủi ro:** nếu nút B có điện trở kéo lên ngoài (kéo GPIO46 lên mức 1 lúc reset) và GPIO0 đang giữ ở 0 thì chip không vào được download mode. Nút B chỉ dùng điện trở kéo lên trong (bật bằng phần mềm sau khi chạy) thì không bị. Bạn kiểm tra giúp nút B có điện trở ngoài không.

## Cấp nguồn trên breadboard

- Tất cả module lấy 3V3 từ DevKit (màn, BNO055, mic, loa). Mass chung.
- Chân 5V đang chỉ có 2.7V vì chưa hàn pad IN-OUT. Không ảnh hưởng vì không module nào dùng 5V.
- MAX98357A chạy từ 3.3V sẽ nhỏ tiếng hơn so với chạy từ pin ở PCB.
