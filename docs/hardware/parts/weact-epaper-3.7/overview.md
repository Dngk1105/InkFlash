# Module e-paper WeAct 3.7"

Màn e-paper đen trắng 240 × 416 điểm ảnh của WeAct Studio. Module gồm một panel 3,7" (controller UC8253 dán trên kính) và một bo mạch nhỏ lo nguồn 3,3 V, mạch tăng áp và đầu nối 8 chân.

Chỉ mục các file chi tiết: `index.md`. Cùng thư mục.

## Ví dụ: một lần vẽ ảnh

ESP32 gửi 12 480 byte ảnh qua SPI (240 × 416 điểm, mỗi điểm 1 bit), gửi lệnh làm mới, rồi chờ chân BUSY lên cao khoảng 2–3 giây. Trong lúc BUSY thấp, màn đảo hạt mực bằng điện áp cao (khoảng ±15 V) do mạch tăng áp trên module tạo ra. Xong thì ảnh đứng yên, không cần cấp điện nữa.

## Thông số chính

Cột "Trạng thái": xem `index.md`.


| Thông số                      | Giá trị                                                                                                                                                     | Nguồn                                                        | Trạng thái     |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------- | ------------------ |
| Loại màn                      | Active Matrix Electrophoretic Display (hạt mực di chuyển trong điện trường, điều khiển bằng ma trận TFT), phản xạ, giữ ảnh không cần điện | GDEY037T03 tr.5 | Đã xác nhận  |
| Kích thước chéo             | 3,7"                                                                                                                                                          | GDEY037T03 tr.5 | Đã xác nhận  |
| Độ phân giải gốc           | 240 (rộng) × 416 (cao), 130 dpi                                                                                                                             | GDEY037T03 tr.5                                                    | Đã xác nhận  |
| Độ phân giải khi xoay ngang | 416 × 240                                                                                                                                                    | Demo `setRotation(1)`                                         | Đã xác nhận  |
| Vùng hiển thị                | 47,04 × 81,54 mm                                                                                                                                             | GDEY037T03 tr.5 | Đã xác nhận  |
| Controller                      | UC8253 (UltraChip), hỗ trợ cả đen/trắng/đỏ, tối đa 240 × 480                                                                                        | UC8253 tr.3 | Đã xác nhận  |
| Số màu                        | Đen/trắng, 1 bit mỗi điểm (chế độ KW của controller)                                                                                                 | GxEPD2 `hasColor = false`                                     | Đã xác nhận |
| Partial refresh                 | Có; làm mới vùng chữ nhật, mép ngang căn theo 8 điểm                                                                                                | UC8253 tr.37 (lệnh `0x90`); GxEPD2 `hasPartialUpdate = true` | Đã xác nhận  |
| Điện áp cấp module          | 3,3–5 V nếu hàn cầu SB4 + SB5 (qua LDO); chỉ 3,3 V nếu hàn cầu SB3                                                                                    | In trên bản vẽ hình dạng; sơ đồ                       | Đã xác nhận  |
| Mức logic tín hiệu           | 3,3 V                                                                                                                                                         | In trên bản vẽ hình dạng                                 | Đã xác nhận  |
| Giao tiếp                      | SPI 4 dây, không nối MISO; đọc được qua chính chân SDA (`interface.md`); ghi MSB trước, lấy mẫu ở cạnh lên của SCL                          | UC8253 tr.41–42; GxEPD2; đo trên BB0                       | Đã xác nhận  |
| Tốc độ SCL dùng được     | Demo chạy 4 MHz; bảng AC ghi chu kỳ ghi tối thiểu 170 ns (khoảng 5,9 MHz) ở VDD = 2,3 V                                                                | UC8253 tr.61                                                  | Đã xác nhận  |
| BUSY                            | Mức thấp = đang bận                                                                                                                                       | UC8253 tr.6 | Đã xác nhận  |
| Nhiệt độ hoạt động        | 0–50 °C                                                                                                                                                     | GDEY037T03 tr.27                                                   | Đã xác nhận  |
| Kích thước bo module         | 105,79 × 53,80 mm, 4 lỗ Ø3,2 mm                                                                                                                            | Bản vẽ hình dạng                                          | Đã xác nhận  |

Tên chân in trên module: BUSY, RES, D/C, CS, SCL, SDA, GND, VCC (chi tiết: `pins.md`). SCL là SCK, SDA là MOSI.

## Tên panel: hai nguồn gọi khác nhau

Panel sử dụng là GDEY037T03, có controller là UC8253 và độ phân giải 240 × 416.

## Chỗ các tài liệu lệch nhau

Panel sử dụng là GDEY037T03 (`source/gdey037t03-spec.pdf`). Spec `source/panel-e037a75-a0-spec.pdf` là của panel khác cùng cỡ, chỉ dùng tham khảo khi GDEY037T03 không ghi. Bản spec GDEY037T03 có front-light và cảm ứng, nên kích thước ngoài và khối lượng trong đó không áp cho module này.


| Chỗ lệch           | Chi tiết                                                                                                                                      | Cách hiểu hiện tại                                                        |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------- |
| Pixel pitch          | GDEY037T03 tr.5 ghi 0,196 × 0,196 mm; E037A75-A0 tr.5 ghi 0,202 × 0,203 mm                                                                 | Dùng số của GDEY037T03                                                  |
| Tốc độ SPI        | UC8253 tr.3 ghi clock tới 20 MHz; bảng AC tr.61 ghi chu kỳ ghi tối thiểu 170 ns                                                           | Tin bảng AC, vì đó là điều kiện thực tế; không có bảng cho 3,3 V |
| Điện áp VCC panel | GDEY037T03 tr.27: 2,3–3,6 V (điển hình 3,0 V), khớp UC8253 tr.59; E037A75-A0 tr.11 ghi 2,4–3,6 V                                                                         | Module cấp 3,3 V nên nằm trong mọi khoảng                 |
| Mạch tăng áp      | GDEY037T03 tr.35 đề xuất MOSFET Si1308EDL, diode MBR0530, cuộn NR3015; sơ đồ WeAct dùng 47 µH, 2,2 Ω, AO3402, B5819W | Thiết kế của WeAct khác mạch tham khảo; chưa đánh giá ảnh hưởng  |

Đối chiếu gốc: `source/gdey037t03-spec.pdf` tr.5, 27, 35; `source/panel-e037a75-a0-spec.pdf` tr.4–6, 10–11, 19 (panel khác, tham khảo); `source/uc8253-datasheet.pdf` tr.3, 61; `source/example-arduino-esp32s3.ino` dòng 30.
