# Module e-paper WeAct 3.7": sơ đồ chân và giao tiếp phần cứng

Module sử dụng panel E037A75-A0 (240×416), driver tích hợp UC8253.

## Header kết nối (8 chân)

Module hỗ trợ 2 dạng cổng: hàng rào đực 2,54 mm và socket JST 1,25 mm 8-pin (Wafer-125W-8P). Cả hai dùng chung sơ đồ chân.

| Chân module | Tên chân | Hướng | Nối ESP32-S3 (Board BB0) | Mô tả và ràng buộc phần cứng |
|---|---|---|---|---|
| 1 | 3V3 / VDD | Nguồn vào | 3V3 Rail | Nguồn cấp 3,3 V. Đi qua LDO ME6231 hoặc cấp thẳng qua cầu hàn SB3. |
| 2 | GND | Nguồn vào | GND | Mass chung hệ thống. |
| 3 | SCL / SCK | Vào (Input) | GPIO4 | Xung nhịp SPI (Mode 0). Tối đa 20 MHz. Có trở đệm 100 Ω. |
| 4 | SDA / MOSI | Vào (Input) | GPIO5 | Dữ liệu SPI từ MCU gửi sang màn. Có trở đệm 100 Ω. |
| 5 | CS | Vào (Input) | GPIO6 | Chọn chip, tích cực mức thấp (0 = chọn). Có trở đệm 100 Ω. |
| 6 | DC / D/C | Vào (Input) | GPIO7 | Chọn lệnh hoặc dữ liệu: 0 = Lệnh (Command), 1 = Dữ liệu (Data). |
| 7 | RES / RST | Vào (Input) | GPIO15 | Reset phần cứng, tích cực mức thấp (0 = reset). Độ rộng xung tối thiểu 50 µs. |
| 8 | BUSY | Ra (Output) | GPIO16 | Báo trạng thái driver: **Mức thấp (0) = Đang bận, Mức cao (1) = Rảnh**. |

## Logic chân BUSY

- Chân BUSY trên header nối trực tiếp tới chân `BUSY_N` của controller UC8253 (qua trở bảo vệ 100 Ω).
- Khi controller đang chạy lệnh tốn thời gian (kéo điện áp cao, quét sóng LUT làm mới màn, nạp OTP), chân BUSY giữ mức 0.
- MCU phải chờ chân BUSY lên mức 1 trước khi gửi lệnh hoặc dữ liệu tiếp theo. Không ngắt quãng khi BUSY đang ở mức 0.

## Cầu hàn cấu hình trên module (Solder Bridges)

| Cầu hàn | Trạng thái mặc định | Tác dụng |
|---|---|---|
| SB1 | Đã hàn (Short) | Chọn chế độ SPI 4 dây (`BS = 0`). Chân BS kéo xuống mass qua trở 5,1 kΩ. |
| SB2 | Hở (Open) | Chọn chế độ SPI 3 dây (`BS = 1`, 9-bit SPI). |
| SB3 | Đã hàn (Short) | Chế độ cấp nguồn 3,3 V trực tiếp: nối thẳng VDD vào đường VDD33 của panel. |
| SB4, SB5 | Hở (Open) | Chế độ cấp nguồn 3,3 V - 5 V: dẫn nguồn qua IC ổn áp LDO ME6231A33M3G trên board. |

## Mạch phụ trợ trên module

- **Booster điện áp cao**: Tích hợp cuộn cảm 47 µH, transistor N-MOS AO3402 và 3 diode Schottky để sinh điện áp nguồn âm/dương lớn ($V_{GH} = +20\text{ V}, V_{GL} = -20\text{ V}$) cho các hạt mực điện di chuyển.
- **Tín hiệu MISO**: Màn e-paper chỉ nhận dữ liệu từ MCU, không có đường MISO trả về.
