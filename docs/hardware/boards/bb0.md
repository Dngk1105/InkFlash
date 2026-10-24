# Board BB0: breadboard

ESP32-S3 DevKitC-1 N16R8 cắm trên breadboard, đấu dây ngày 2026-10-21.

Cột "Trạng thái": `Đã xác nhận` (đã chạy thử được với cách đấu đó) hoặc `Chưa xác nhận`.

## Linh kiện

| Khối | Linh kiện (thư mục trong `docs/hardware/parts/`) |
|---|---|
| MCU | `esp32-s3-wroom-1` (trên DevKitC-1) |
| Màn | `weact-epaper-3.7` |
| IMU | `bno055` |
| Mic | `inmp441` |
| Amp + loa | `max98357a`, `speaker-8ohm-1w` |
| LED | `ws2812` (có sẵn trên DevKit) |
| Nút | 2 nút bấm |

## Chân

| Chức năng | GPIO | Ghi chú | Trạng thái |
|---|---|---|---|
| Nút A | GPIO8 | RTC GPIO, đánh thức được. Giả định nối GND khi bấm | Chưa xác nhận |
| Nút B | GPIO46 | Không phải RTC GPIO. Giả định nối GND khi bấm | Chưa xác nhận |
| Màn SCK (module ghi SCL) | GPIO4 | | Đã xác nhận |
| Màn MOSI (module ghi SDA) | GPIO5 | | Đã xác nhận |
| Màn CS | GPIO6 | | Đã xác nhận |
| Màn DC | GPIO7 | | Đã xác nhận |
| Màn RST (module ghi RES) | GPIO15 | | Đã xác nhận |
| Màn BUSY | GPIO16 | | Đã xác nhận |
| I2C SDA | GPIO17 | Chỉ có BNO055 | Chưa xác nhận |
| I2C SCL | GPIO18 | 100 kHz | Chưa xác nhận |
| I2S BCLK | GPIO9 | Dùng chung: mic SCK và loa BCLK | Chưa xác nhận |
| I2S WS | GPIO10 | Dùng chung: mic WS và loa LRC | Chưa xác nhận |
| I2S mic DIN | GPIO11 | INMP441 SD | Chưa xác nhận |
| I2S loa DOUT | GPIO12 | MAX98357A DIN | Chưa xác nhận |
| LED RGB | GPIO48 | WS2812 | Chưa xác nhận |
| Màn MISO | | Không dùng: màn chỉ ghi | |
| Còn trống | GPIO13, GPIO14 | | |

Chân màn đã xác nhận bằng bản demo Arduino.

## Lưu ý riêng của board này

- **Nút B không đánh thức được máy**, vì GPIO46 không phải RTC GPIO. Chỉ nút A đánh thức; nút B chỉ dùng khi máy
  đang thức. Mọi board khác: cả hai nút phải là RTC GPIO.
- GPIO46 là chân strapping: nút B chỉ được dùng điện trở kéo lên trong, không dùng điện trở kéo lên ngoài, để không
  chặn download mode.

## Nguồn

- Mọi module lấy 3V3 từ DevKit (màn, BNO055, mic, loa), mass chung.
- Chân 5V chỉ có 2,7 V vì chưa hàn pad IN-OUT. Không ảnh hưởng, vì không module nào dùng 5V.
- MAX98357A chạy từ 3V3 nên nhỏ tiếng hơn so với chạy từ pin.
