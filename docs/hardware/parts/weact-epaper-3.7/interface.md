# Module e-paper WeAct 3.7": giao tiếp SPI, reset và BUSY

Cách ESP32 nói chuyện với controller UC8253: khung SPI, thời gian tối thiểu, reset, chân BUSY và deep sleep. Thứ tự lệnh nằm ở `sequences.md`.

## Ví dụ: gửi một byte lệnh rồi một byte dữ liệu

Gửi lệnh `0x12` (Display Refresh): kéo D/C xuống thấp, kéo CS xuống thấp, đẩy 8 bit từ bit 7 xuống bit 0, mỗi bit lấy mẫu ở cạnh lên của SCL, rồi kéo CS lên cao. Gửi byte dữ liệu: làm đúng như vậy nhưng D/C ở mức cao. Mã mẫu của hãng làm đúng thế, và nâng CS lên giữa từng byte (`source/panel-vendor-init-uc8253.c` dòng 102–149).

## Khung SPI 4 dây

| Hạng mục | Quy định | Nguồn | Trạng thái |
|---|---|---|---|
| Chọn chip | CSB thấp = chọn; datasheet khuyên kéo CSB lên cao sau mỗi 8 bit để chống nhiễu, bộ đếm bit được xoá ở cạnh lên của CSB | UC8253 tr.42 | Đã xác nhận |
| Phân biệt lệnh/dữ liệu | Chân DC: thấp = lệnh, cao = dữ liệu | UC8253 tr.7 | Đã xác nhận |
| Thứ tự bit | MSB trước, lấy mẫu ở cạnh lên SCL | UC8253 tr.41 | Đã xác nhận |
| Xung SCL lúc nghỉ | Mức thấp (mã mẫu hãng kéo SCL xuống trước khi truyền) | Mã mẫu hãng | Đã xác nhận |
| Tham số thừa | Controller chỉ nhận đúng số tham số định nghĩa, bỏ phần dư | UC8253 tr.41 | Đã xác nhận |
| Khi nào lệnh có hiệu lực | Lệnh được xử lý ở điều kiện "stop" của giao tiếp (CS lên cao) | UC8253 tr.13 ghi chú 3 | Đã xác nhận |
| Lệnh đọc | Datasheet có (ví dụ `0x71`, `0x70`), dữ liệu ra trên SDA; module chỉ có một dây SDA | UC8253 tr.42 | Chưa xác nhận (chưa thử trên module) |

## Thời gian SPI tối thiểu

Điều kiện đo của hãng là VDD = 2,3 V (UC8253 tr.61). Ở 3,3 V không có bảng riêng, nên các số dưới đây là giới hạn an toàn.

| Ký hiệu | Ý nghĩa | Tối thiểu (ns) |
|---|---|---|
| TCSS | Thiết lập CSB trước xung SCL đầu | 60 |
| TCSH | Giữ CSB sau xung SCL cuối | 65 |
| TSCC | Thiết lập CSB (cạnh lên) | 20 |
| TCHW | CSB ở mức cao giữa hai lần truyền | 40 |
| TSCYCW | Chu kỳ SCL khi ghi (tương đương khoảng 5,9 MHz) | 170 |
| TSHW, TSLW | SCL mức cao, SCL mức thấp khi ghi | 85, 85 |
| TDCS, TDCH | Thiết lập, giữ chân DC | 30, 30 |
| TSDS, TSDH | Thiết lập, giữ dữ liệu SDA | 30, 30 |
| TSCYCR | Chu kỳ SCL khi đọc | 350 |
| TACC | Thời gian truy cập SDA khi đọc (tối đa) | 250 |

Demo chạy SCL 4 MHz (chu kỳ 250 ns) nằm trong giới hạn này.

## SPI 3 dây (không dùng trên module này)

Khi BS ở mức cao, controller chuyển sang 3 dây: mỗi lần truyền 9 bit, bit đầu cho biết lệnh hay dữ liệu, chân DC phải nối GND (UC8253 tr.41). Cầu hàn chọn chế độ nằm ở `pins.md`.

## Reset

| Hạng mục | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Reset phần cứng | RST_N mức thấp, ít nhất 50 µs | UC8253 tr.6 | Đã xác nhận |
| Chờ sau khi RST_N lên cao | Lớn hơn 1 ms; lệnh gửi trong lúc này bị bỏ qua | UC8253 tr.43 | Đã xác nhận |
| Trình tự dùng ở demo GxEPD2 | Cao 10 ms, thấp 50 ms, cao 50 ms | Demo | Đã xác nhận |
| Trình tự dùng ở mã mẫu hãng | Cao 1 ms, thấp 5 ms, cao 1 ms | Mã mẫu hãng dòng 78 | Đã xác nhận |
| Reset mềm | Gửi lệnh `0x00` với bit RST_N = 0; mất ít nhất 50 µs, BUSY thấp, lệnh khác bị bỏ qua | UC8253 tr.14 | Đã xác nhận |
| Hiệu ứng của reset | Mọi thanh ghi về mặc định, mọi chức năng tắt | UC8253 tr.6 | Đã xác nhận |
| Vùng ảnh cũ sau reset | Không còn khớp với ảnh đang hiện (datasheet không nói rõ RAM có bị xoá không) | Demo; `image-memory.md` | Chưa xác nhận |

## Chân BUSY

| Hạng mục | Giá trị | Nguồn |
|---|---|---|
| Mức | Thấp = đang bận; cao = sẵn sàng nhận lệnh | UC8253 tr.6 |
| Lệnh làm BUSY xuống thấp | `0x02` POF, `0x04` PON, `0x05` PMES, `0x07` DSLP, `0x12` DRF, `0x11` DSP, `0x17` AUTO, `0x2A`, `0x44`, `0x65`, `0x90` | UC8253 tr.46 |
| Lệnh không đặt BUSY | PSR, PWR, PFS, BTST, DTM1, DTM2, các lệnh LUT, CDI, TCON... | UC8253 tr.46 |
| Trong lúc làm mới | Hầu hết lệnh bị bỏ qua; riêng TCON, TRES, REV được nhận | UC8253 tr.46 |
| Chờ sau DRF | Từ lúc BUSY xuống thấp tới lệnh đọc `0x71` đầu tiên phải lớn hơn 200 µs | UC8253 tr.20 |
| Cấu hình chân ESP32 | Đầu vào kéo lên trong | Demo |

Trước khi gửi lệnh có đánh dấu "Flag", chờ BUSY cao rồi mới gửi lệnh tiếp. Mã mẫu hãng làm đúng thế (hàm `READBUSY`, dòng 90–98).

## Deep sleep

| Hạng mục | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Vào deep sleep | Lệnh `0x07` kèm tham số `0xA5` | UC8253 tr.20 | Đã xác nhận |
| Thoát deep sleep | Chỉ bằng reset phần cứng (RST_N hạ xuống) | UC8253 tr.20, 53 | Đã xác nhận |
| Khi đang ngủ | Nên đưa các chân điều khiển về 0 V để tránh dòng rò IO | UC8253 tr.53 | Đã xác nhận |
| Ánh sáng | Tránh để chip lộ ra ánh sáng: hiệu ứng quang điện làm các nút bên trong mất ổn định | UC8253 tr.53 | Đã xác nhận |
| Sau khi ngủ dậy | Vùng ảnh cũ không còn khớp ảnh đang hiện; demo xử lý bằng cách xoá trắng toàn màn (xem `image-memory.md`) | Demo | Đã xác nhận |

Đối chiếu gốc: `source/uc8253-datasheet.pdf` tr.6–7, 13–14, 20, 41–46, 53, 60–61; `source/panel-vendor-init-uc8253.c` dòng 78–150.
