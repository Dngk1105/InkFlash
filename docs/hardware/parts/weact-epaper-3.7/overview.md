# Module e-paper WeAct 3.7"

Màn e-paper đen trắng 240 × 416 điểm ảnh của WeAct Studio. Module gồm một panel 3,7" (controller UC8253 dán trên kính) và một bo mạch nhỏ lo nguồn 3,3 V, mạch tăng áp và đầu nối 8 chân.

Chỉ mục các file chi tiết: `index.md`. Cùng thư mục.

## Ví dụ: một lần vẽ ảnh

ESP32 gửi 12 480 byte ảnh qua SPI (240 × 416 điểm, mỗi điểm 1 bit), gửi lệnh làm mới, rồi chờ chân BUSY lên cao khoảng 2–3 giây. Trong lúc BUSY thấp, màn đảo hạt mực bằng điện áp cao (khoảng ±15 V) do mạch tăng áp trên module tạo ra. Xong thì ảnh đứng yên, không cần cấp điện nữa.

## Thông số chính

Cột "Trạng thái": xem `index.md`.

| Thông số | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Loại màn | Active Matrix Electrophoretic Display (hạt mực di chuyển trong điện trường, điều khiển bằng ma trận TFT), phản xạ, giữ ảnh không cần điện | Panel tr.4 | Đã xác nhận |
| Kích thước chéo | 3,7" | Panel tr.5 | Đã xác nhận |
| Độ phân giải gốc | 240 (rộng) × 416 (cao), 129 dpi | Panel tr.5 | Đã xác nhận |
| Độ phân giải khi xoay ngang | 416 × 240 | Demo `setRotation(1)` | Đã xác nhận |
| Vùng hiển thị | 47,04 × 81,54 mm | Panel tr.5 | Đã xác nhận |
| Controller | UC8253 (UltraChip), hỗ trợ cả đen/trắng/đỏ, tối đa 240 × 480 | Panel tr.6 ghi chú 2; UC8253 tr.3 | Đã xác nhận |
| Số màu | Đen/trắng, 1 bit mỗi điểm (chế độ KW của controller) | GxEPD2 `hasColor = false` | Chưa xác nhận |
| Partial refresh | Có; làm mới vùng chữ nhật, mép ngang căn theo 8 điểm | UC8253 tr.37 (lệnh `0x90`); GxEPD2 `hasPartialUpdate = true` | Đã xác nhận |
| Điện áp cấp module | 3,3–5 V nếu hàn cầu SB4 + SB5 (qua LDO); chỉ 3,3 V nếu hàn cầu SB3 | In trên bản vẽ hình dạng; sơ đồ | Đã xác nhận |
| Mức logic tín hiệu | 3,3 V | In trên bản vẽ hình dạng | Đã xác nhận |
| Giao tiếp | SPI 4 dây, không nối MISO; đọc được qua chính chân SDA (`interface.md`); ghi MSB trước, lấy mẫu ở cạnh lên của SCL | UC8253 tr.41–42; GxEPD2; đo trên BB0 | Đã xác nhận |
| Tốc độ SCL dùng được | Demo chạy 4 MHz; bảng AC ghi chu kỳ ghi tối thiểu 170 ns (khoảng 5,9 MHz) ở VDD = 2,3 V | UC8253 tr.61 | Đã xác nhận |
| BUSY | Mức thấp = đang bận | UC8253 tr.6; Panel tr.8 | Đã xác nhận |
| Nhiệt độ hoạt động | 0–40 °C | Panel tr.11 | Đã xác nhận |
| Kích thước bo module | 105,79 × 53,80 mm, 4 lỗ Ø3,2 mm | Bản vẽ hình dạng | Đã xác nhận |

Tên chân in trên module: BUSY, RES, D/C, CS, SCL, SDA, GND, VCC (chi tiết: `pins.md`). SCL là SCK, SDA là MOSI.

## Tên panel: hai nguồn gọi khác nhau

Hai nguồn trong tay gọi panel bằng hai tên khác nhau, nhưng đều nói controller là UC8253 và độ phân giải 240 × 416.

| Nguồn | Tên panel | Ghi chú |
|---|---|---|
| GxEPD2 và ví dụ Arduino của WeAct (`source/example-arduino-esp32s3.ino` dòng 30) | GDEY037T03 (Good Display) | Tên được docs trước đây dùng |
| Thư mục tài liệu hãng của WeAct, mục `3.7 Inch Black&Write` (`source/panel-e037a75-a0-spec.pdf`) | E037A75-A0 của Shenzhen Yingruida Optoelectronics (英瑞达), kính TFT của BOE (FL037YYQ-N51) | Bản phát hành 2025-06-30 |

Chưa biết module đang cầm trên tay là panel nào. Cách kiểm tra: đọc mã QR hoặc nhãn sau panel; tài liệu hãng giải thích cấu trúc mã ở Panel tr.19 (9 ký tự sau ký tự đầu là mã model, ví dụ `E037A75**`).

## Chỗ các tài liệu lệch nhau

| Chỗ lệch | Chi tiết | Cách hiểu hiện tại |
|---|---|---|
| Pixel pitch | Panel tr.5 ghi 0,202 × 0,203 mm; bản vẽ Panel tr.6 ghi 0,196 × 0,196 mm | Không dùng để tính toán; chưa rõ số nào đúng |
| Màu | Panel tr.4 mô tả "B/W/R full display capabilities", nhưng tên file và thư mục ghi `BW` | Module của WeAct là đen trắng; chưa thử điều khiển đỏ |
| Tốc độ SPI | UC8253 tr.3 ghi clock tới 20 MHz; bảng AC tr.61 ghi chu kỳ ghi tối thiểu 170 ns | Tin bảng AC, vì đó là điều kiện thực tế; không có bảng cho 3,3 V |
| Điện áp VCC panel | Panel tr.11: 2,4–3,6 V (điển hình 3,0 V); UC8253 tr.59: 2,3–3,6 V | Module cấp 3,3 V cho cả hai nên nằm trong cả hai khoảng |
| Mạch tăng áp | Panel tr.10 đề xuất cuộn 10 µH, điện trở 0,47 Ω, MOSFET Si1308EDL, diode MBR0530; sơ đồ WeAct dùng 47 µH, 2,2 Ω, AO3402, B5819W | Thiết kế của WeAct khác mạch tham khảo; chưa đánh giá ảnh hưởng |

Đối chiếu gốc: `source/panel-e037a75-a0-spec.pdf` tr.4–6, 10–11, 19; `source/uc8253-datasheet.pdf` tr.3, 61; `source/example-arduino-esp32s3.ino` dòng 30.
