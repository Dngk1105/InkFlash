# Phần cứng

Mỗi linh kiện một thư mục trong `parts/`, mỗi board một file trong `boards/`. Chỉ mở file cần.

- `parts/<linh-kiện>/overview.md`: linh kiện là gì, thông số chính, đã chốt cho sản phẩm hay chỉ dùng trên breadboard.
- `parts/<linh-kiện>/<đặc-tính>.md`: từng đặc tính riêng (chân, lệnh, thời gian...), chỉ có khi cần.
- `boards/<mã-board>.md`: board dùng linh kiện nào, đấu chân nào, lưu ý riêng.

## Linh kiện

| Thư mục | Vai trò | File |
|---|---|---|
| `esp32-s3-wroom-1` | MCU | `overview`, `pins` (ràng buộc chân, strapping) |
| `weact-epaper-3.7` | Màn e-paper (controller UC8253; panel GDEY037T03 hoặc E037A75, xem overview) | Bắt đầu từ `index` (chỉ mục tra cứu). `overview`, `pins`, `electrical`, `mechanical`, `interface` (SPI, reset, BUSY), `image-memory`, `commands`, `waveform`, `sequences`, `refresh`, `glossary`, `sources`; PDF gốc trong `source/` |
| `bq25180` | Sạc pin, ship mode | `overview` |
| `rt9080` | LDO 3,3 V | `overview` |
| `tps22916` | Công tắc tải cho khối phụ | `overview` |
| `lipo-503450` | Pin | `overview` |
| `max98357a` | Amp loa | `overview` |
| `speaker-8ohm-1w` | Loa | `overview` |
| `bno055` | IMU, chỉ trên breadboard | `overview` |
| `inmp441` | Mic, chỉ trên breadboard | `overview` |
| `ws2812` | LED RGB trên DevKit | `overview` |

Không dùng MPU6050 làm IMU.

## Board

| File | Board |
|---|---|
| `boards/bb0.md` | Breadboard: DevKitC-1 N16R8 + module rời |
