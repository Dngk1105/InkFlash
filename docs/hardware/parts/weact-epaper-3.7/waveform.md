# Module e-paper WeAct 3.7": dạng sóng, bảng LUT và nhiệt độ

Màn đổi hình bằng cách cấp một chuỗi điện áp lên từng điểm ảnh. Chuỗi đó gọi là dạng sóng, lưu trong các bảng LUT. Controller chọn bảng theo nhiệt độ. File này giải thích cách chọn và cách đọc bảng, để hiểu vì sao lệnh `0xE0` và `0xE5` đổi hình ảnh trông thế nào.

## Ví dụ: một điểm ảnh từ trắng thành đen

Ảnh cũ ghi điểm đó là trắng, ảnh mới ghi là đen. Controller tra bảng "trắng thành đen" (LUTWK). Bảng quy định, theo từng khung hình, điểm đó nhận mức điện áp nào (VSH, VSL hoặc 0 V), và trong lúc đó điện cực chung (VCOM) đổi mức theo bảng VCOM (LUTC). Về nguyên lý chung của công nghệ này, điện trường giữa điểm ảnh và VCOM kéo các hạt mực mang điện lên hoặc xuống mặt kính, và điểm ảnh giữ nguyên trạng thái khi tắt điện.

Số khung hình và mức điện áp của mỗi pha phụ thuộc nhiệt độ: datasheet cho phép tới 17 dải nhiệt độ, mỗi dải một bộ bảng riêng (phần dưới).

## Cấu trúc một bảng LUT

| Khái niệm | Ý nghĩa | Nguồn |
|---|---|---|
| Nhóm (group) | Một đoạn của dạng sóng; chế độ KW có 6 nhóm, KWR có 8; mỗi nhóm 7 byte | UC8253 tr.22–23 |
| Lặp nhóm (group repeat) | 0–255 lần lặp lại cả nhóm; byte đầu của nhóm | UC8253 tr.22 |
| Trạng thái (state) | Mỗi nhóm có 2 trạng thái, mỗi trạng thái 2 pha | UC8253 tr.22 |
| Pha (phase) | Mỗi pha gồm mức điện áp (2 bit) và số khung hình (6 bit, tối đa 63) | UC8253 tr.22 |
| Lặp trạng thái | 2 byte cuối: số lần lặp state 1 và state 2 (0–255) | UC8253 tr.22 |
| Cách tính lặp | Giá trị lặp n nghĩa là chạy đúng n lần. Thời gian một lần DRF = (tổng số khung của bảng dài nhất + 2) / tần số khung | Đo trên BB0, 2026-10-30: số khung tính từ OTP khớp DRF đo được của TR4–TR8, lệch dưới 1 % (bảng ở phần OTP) |

Mức điện áp (2 bit) trong bảng nguồn (LUTWW, LUTKW, LUTWK, LUTKK): `00` = 0 V; `01` = VSH; `10` = VSL; `11` = VDHR (UC8253 tr.23). Trong bảng VCOM (LUTC): `00` = VCOM_DC; `01` = VSH + VCOM_DC; `10` = VSL − VCOM_DC; `11` = nổi (UC8253 tr.22).

Các bảng độc lập nhau. Nếu độ dài khác nhau, controller lấy bảng dài nhất làm thời gian làm mới và cấp 0 V cho phần còn lại của các bảng ngắn (UC8253 tr.24). Sau khi kết thúc, controller thả hai khung VCOM và dữ liệu về 0 V (UC8253 tr.44–45).

Chế độ KW dùng năm bảng: LUTC (VCOM), LUTWW, LUTKW, LUTWK, LUTKK. Tên viết theo "từ → sang": K là đen, W là trắng.

## Bảng sóng nằm ở đâu: OTP hay thanh ghi

| Cách | Điều kiện | Ghi chú |
|---|---|---|
| Từ OTP | Bit REG của lệnh `0x00` = 0 (mặc định) | OTP trong controller chứa sẵn các bảng sóng; sau DSP/DRF bảng được chép sang thanh ghi tự động (UC8253 tr.43) |
| Từ thanh ghi | REG = 1 | Host tự ghi bằng lệnh `0x20`–`0x24` trước khi làm mới |

Hai trình tự mẫu (GxEPD2, mã mẫu hãng) đều để REG = 0, tức dùng bảng trong OTP.

Đường thanh ghi đã chạy trên BB0 (2026-10-30):

| Hạng mục | Kết quả |
|---|---|
| Nạp lại đúng bảng TR8 đọc từ OTP, kèm điện áp, VCOM, tần số khung của TR8, rồi đặt REG = 1 | DRF 343,1 ms, so với 344,6 ms khi dùng OTP |
| Số byte gửi | `0x20`, `0x22`–`0x24`: 56 byte (6 nhóm dùng, 2 nhóm cuối bằng 0); `0x21`: 42 byte |
| Lệnh `0x30` (PLL) khi REG = 1 | Có tác dụng: 15 khung ở 100 Hz cho DRF 171,8 ms, ở 200 Hz cho 86,1 ms |
| Cắt bảng còn N khung | DRF đúng (N + 2) khung; số đo ở `refresh.md` |

## OTP và dải nhiệt độ

OTP của controller có 8 KB, chia thành hai bank 4 KB để lập trình hai lần (UC8253 tr.47). Mỗi bank chứa:

| Nội dung | Số lượng | Ghi chú |
|---|---|---|
| Giá trị mặc định thay thế cho thanh ghi lệnh | 19 byte (gồm khoá bật `0xA5`) | Giảm số byte host phải gửi khi khởi tạo (UC8253 tr.51) |
| Ngưỡng nhiệt độ TB0–TB15 | 16 byte, mỗi byte là °C có dấu | Kết thúc sớm bằng giá trị `0x7F` |
| Dải nhiệt độ TR0–TR16 | Tối đa 17 dải; mỗi dải có tốc độ khung, điện áp VSH/VSL/VDHR/VGH/VGL, VCOM_DC, XON và các bảng LUT | UC8253 tr.47, 52 |

Khi bắt đầu làm mới (DRF hoặc DSP), controller đọc nhiệt độ rồi so lần lượt: nếu nhiệt độ ≤ TB0 thì dùng TR0, ngược lại nếu ≤ TB1 thì dùng TR1, tiếp tục như vậy; nếu lớn hơn ngưỡng cuối thì dùng dải cuối (UC8253 tr.48–50). Bank 0 được dùng nếu địa chỉ `0x000` chứa `0xA5`, nếu không thì kiểm bank 1; cả hai không có thì không làm mới. Datasheet ghi địa chỉ kiểm bank 1 là `0x1000` ở bảng bản đồ OTP (tr.47) nhưng `0xC00` ở bảng thủ tục (tr.48); hai chỗ lệch nhau.

Ví dụ trong datasheet (UC8253 tr.49) chỉ để minh hoạ cơ chế, không phải nội dung OTP của panel này: với các ngưỡng −15, −5, 0, 10, 30 °C thì −20 °C dùng TR0, 0 °C dùng TR2, 20 °C dùng TR4, 40 °C và hơn dùng TR5.

## Nội dung OTP của module này

Đọc bằng lệnh `0xA2` qua chân SDA trên BB0, 2026-10-30, hai lần khớp nhau (`interface.md`). Bank 0 có khoá `0xA5` và là bank đang dùng; bank 1 trống (`FF`).

Mỗi dải TR dài `0xE9` byte, TR0 bắt đầu ở `0x002A` (bố cục KW ở UC8253 tr.52): tần số khung, VG, VSH, VSL, VDHR, VCOM_DC, EOPT, XON (2 byte), rồi LUTC, LUTWW, LUTKW, LUTWK, LUTKK, mỗi bảng 42 byte.

| Hạng mục | Giá trị |
|---|---|
| Giá trị mặc định thay cho lệnh (`0x001`–`0x013`) | `A5 D7 00 D7 D7 27 00 97 22 F0 01 A0 00 00 00 00 88 03 00` |
| Ngưỡng TB0–TB8 | `05 0A 0F 14 1E 50 5C 64 7F` (5, 10, 15, 20, 30, 80, 92, 100, 127 °C); TB9–TB15 là `FF` |
| Dải dùng được | TR0–TR8; TR9–TR16 trống |
| Tần số khung mọi dải | `09` = 50 Hz |
| Điện áp mọi dải | VG `10`, VSH `3F`, VSL `3F` (±15 V, GDEY037T03 tr.13), VDHR `14` |
| VCOM_DC | `13` ở TR0–TR7, `09` ở TR8 |

Số khung từng dải, tính theo cách tính lặp ở trên, so với DRF đo trước đây (`refresh.md`):

| Nhiệt độ ép (`E5`) | Dải | Số khung + 2 | Thời gian tính | DRF đo |
|---|---|---|---|---|
| 30 | TR4 (≤ 30 °C) | 126 | 2 520 ms | 2 538 ms |
| 50, 70 | TR5 (≤ 80 °C) | 129 | 2 580 ms | 2 598 ms |
| 90 | TR6 (≤ 92 °C) | 102 | 2 040 ms | 2 055 ms |
| 100 | TR7 (≤ 100 °C) | 74 | 1 480 ms | 1 492 ms |
| 110 | TR8 (≤ 127 °C) | 17 | 340 ms | 344 ms |

TR8, tức sóng partial mà driver đang dùng, chỉ có một nhóm:

| Bảng | Nội dung | Nghĩa |
|---|---|---|
| LUTC | VCOM_DC 14 khung, VCOM_DC 1 khung | VCOM đứng yên |
| LUTWW | 0 V 15 khung | Điểm trắng giữ trắng không bị cấp điện |
| LUTKW | VSL 15 khung | Kéo về trắng |
| LUTWK | VSH 15 khung | Kéo về đen |
| LUTKK | 0 V 14 khung, VSH 1 khung | Điểm đen giữ đen được kéo thêm 1 khung |

## Nhiệt độ lấy từ đâu

| Nguồn | Cách bật | Ghi chú |
|---|---|---|
| Cảm biến trong controller | Mặc định (TSE = 0 ở lệnh `0x41`) | −25–50 °C, sai số ±2 °C; đo tự động một lần khi PON nếu TS_AUTO = 1 (UC8253 tr.3, 15, 19) |
| Cảm biến ngoài LM75 qua I2C | TSE = 1 | Chân TSCL, TSDA không nối trên module nên không dùng được |
| Đặt tay | Lệnh `0xE0` bit TSFIX = 1, rồi lệnh `0xE5` đặt giá trị | Datasheet viết cho trường hợp ghép nối chip (cascade) nhưng thanh ghi nằm trong chip nào cũng có (UC8253 tr.39–40) |

Có thể bù nhiệt độ cảm biến trong bằng tham số TO[3:0] của lệnh `0x41` (từ −8 đến +7 °C).

## Ý nghĩa `E0 02` và `E5 xx` trong các trình tự mẫu

| Lệnh | Giải mã theo datasheet | Trạng thái |
|---|---|---|
| `E0 02` | Bit D1 = TSFIX = 1: dùng nhiệt độ đặt tay | Đã xác nhận (UC8253 tr.39) |
| `E5 5A` (full refresh của GxEPD2) | TS_SET = `0x5A` = 90 theo mã "số nguyên °C có dấu" của bảng tr.27 (ví dụ `0x19` = 25 °C); chọn TR6, sóng full 102 khung | Đã xác nhận (OTP đọc trên BB0) |
| `E5 6E` (partial của GxEPD2) | 110; chọn TR8, sóng partial 17 khung | Như trên |
| `E5 53` (mã mẫu hãng, "slow update") | 83; chọn TR6 | Như trên |

Ba giá trị đều vượt xa nhiệt độ làm việc: chúng là cách chọn một dải sóng cụ thể trong OTP, không phải nhiệt độ thật. TR8 là dải ngắn nhất trong OTP, nên với sóng OTP thì không có partial nào nhanh hơn. Kiểu cập nhật nhanh của hãng (`dis_img_fast`) không gửi `E0/E5` mà để controller đo nhiệt độ thật.

Kết quả đo từ demo: bỏ lệnh ép nhiệt độ cho full refresh sạch hơn và chậm hơn khoảng 0,5 s (xem `refresh.md`).

Đối chiếu gốc: `source/uc8253-datasheet.pdf` tr.15, 19, 22–28, 39–40, 43–52; `source/gdey037t03-spec.pdf` tr.13.
