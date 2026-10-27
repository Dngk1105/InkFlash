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

Mức điện áp (2 bit) trong bảng nguồn (LUTWW, LUTKW, LUTWK, LUTKK): `00` = 0 V; `01` = VSH; `10` = VSL; `11` = VDHR (UC8253 tr.23). Trong bảng VCOM (LUTC): `00` = VCOM_DC; `01` = VSH + VCOM_DC; `10` = VSL − VCOM_DC; `11` = nổi (UC8253 tr.22).

Các bảng độc lập nhau. Nếu độ dài khác nhau, controller lấy bảng dài nhất làm thời gian làm mới và cấp 0 V cho phần còn lại của các bảng ngắn (UC8253 tr.24). Sau khi kết thúc, controller thả hai khung VCOM và dữ liệu về 0 V (UC8253 tr.44–45).

Chế độ KW dùng năm bảng: LUTC (VCOM), LUTWW, LUTKW, LUTWK, LUTKK. Tên viết theo "từ → sang": K là đen, W là trắng.

## Bảng sóng nằm ở đâu: OTP hay thanh ghi

| Cách | Điều kiện | Ghi chú |
|---|---|---|
| Từ OTP | Bit REG của lệnh `0x00` = 0 (mặc định) | OTP trong controller chứa sẵn các bảng sóng; sau DSP/DRF bảng được chép sang thanh ghi tự động (UC8253 tr.43) |
| Từ thanh ghi | REG = 1 | Host tự ghi bằng lệnh `0x20`–`0x24` trước khi làm mới |

Hai trình tự mẫu (GxEPD2, mã mẫu hãng) đều để REG = 0, tức dùng bảng trong OTP.

## OTP và dải nhiệt độ

OTP của controller có 8 KB, chia thành hai bank 4 KB để lập trình hai lần (UC8253 tr.47). Mỗi bank chứa:

| Nội dung | Số lượng | Ghi chú |
|---|---|---|
| Giá trị mặc định thay thế cho thanh ghi lệnh | 19 byte (gồm khoá bật `0xA5`) | Giảm số byte host phải gửi khi khởi tạo (UC8253 tr.51) |
| Ngưỡng nhiệt độ TB0–TB15 | 16 byte, mỗi byte là °C có dấu | Kết thúc sớm bằng giá trị `0x7F` |
| Dải nhiệt độ TR0–TR16 | Tối đa 17 dải; mỗi dải có tốc độ khung, điện áp VSH/VSL/VDHR/VGH/VGL, VCOM_DC, XON và các bảng LUT | UC8253 tr.47, 52 |

Khi bắt đầu làm mới (DRF hoặc DSP), controller đọc nhiệt độ rồi so lần lượt: nếu nhiệt độ ≤ TB0 thì dùng TR0, ngược lại nếu ≤ TB1 thì dùng TR1, tiếp tục như vậy; nếu lớn hơn ngưỡng cuối thì dùng dải cuối (UC8253 tr.48–50). Bank 0 được dùng nếu địa chỉ `0x000` chứa `0xA5`, nếu không thì kiểm bank 1; cả hai không có thì không làm mới. Datasheet ghi địa chỉ kiểm bank 1 là `0x1000` ở bảng bản đồ OTP (tr.47) nhưng `0xC00` ở bảng thủ tục (tr.48); hai chỗ lệch nhau.

Ví dụ trong datasheet (UC8253 tr.49) chỉ để minh hoạ cơ chế, không phải nội dung OTP của panel này: với các ngưỡng −15, −5, 0, 10, 30 °C thì −20 °C dùng TR0, 0 °C dùng TR2, 20 °C dùng TR4, 40 °C và hơn dùng TR5.

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
| `E5 5A` (full refresh của GxEPD2) | TS_SET = `0x5A` = 90 theo mã "số nguyên °C có dấu" của bảng tr.27 (ví dụ `0x19` = 25 °C) | Đã xác nhận cách giải mã; ý đồ chưa rõ |
| `E5 6E` (partial của GxEPD2) | 110 | Như trên |
| `E5 53` (mã mẫu hãng, "slow update") | 83 | Như trên |

Ba giá trị khác nhau (83, 90, 110) đều vượt xa nhiệt độ làm việc. Chưa biết giá trị nào chọn dải TR nào, vì không biết ngưỡng TB trong OTP của panel. Có thể đọc OTP bằng lệnh `0xA2` (ROTP) để biết, nhưng chưa làm. Đã biết: cùng cơ chế "ép nhiệt độ" thì GxEPD2 dùng hai giá trị khác nhau cho full và partial, và mã mẫu hãng dùng thêm một giá trị thứ ba cho kiểu cập nhật chậm; còn kiểu cập nhật nhanh của hãng (`dis_img_fast`) không gửi `E0/E5` mà để controller đo nhiệt độ thật.

Kết quả đo từ demo: bỏ lệnh ép nhiệt độ cho full refresh sạch hơn và chậm hơn khoảng 0,5 s (xem `refresh.md`).

Đối chiếu gốc: `source/uc8253-datasheet.pdf` tr.15, 19, 22–28, 39–40, 43–52.
