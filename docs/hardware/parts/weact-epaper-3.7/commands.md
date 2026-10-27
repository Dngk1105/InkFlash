# Module e-paper WeAct 3.7": bảng lệnh controller UC8253

Tra cứu lệnh theo mã: tên, tham số, giá trị mặc định, tác dụng. Lệnh nào do bộ nhớ ảnh, dạng sóng hoặc nguồn chi phối thì có file riêng (`image-memory.md`, `waveform.md`, `sequences.md`).

## Ví dụ: đọc một dòng của bảng

Dòng `0x04 | PON | không tham số | bật nguồn` nghĩa là: gửi đúng một byte lệnh `0x04` (D/C thấp), không có byte dữ liệu theo sau. Controller bật mạch tăng áp rồi kéo BUSY xuống thấp cho tới khi các điện áp ổn định.

Các cột "Dùng" ghi lệnh đó có xuất hiện ở trình tự của GxEPD2 / demo (`G`) hoặc mã mẫu hãng (`H`).

## Những lệnh chính cho màn đen trắng

| Mã | Tên | Tham số (số byte) | Mặc định | Tác dụng | Dùng | Datasheet |
|---|---|---|---|---|---|---|
| `0x00` | PSR, Panel Setting | 2 | `0x0F`, `0x8D` | Chế độ KW/KWR, hướng quét, reset mềm, độ phân giải sơ bộ | G, H | tr.14–15 |
| `0x04` | PON, Power ON | 0 | | Bật tăng áp; BUSY thấp tới khi sẵn sàng | G, H | tr.19 |
| `0x02` | POF, Power OFF | 0 | | Tắt tăng áp, giữ thanh ghi | G, H | tr.18 |
| `0x07` | DSLP, Deep Sleep | 1 (`0xA5`) | | Ngủ sâu; thoát bằng reset phần cứng | G, H | tr.20 |
| `0x10` | DTM1 | 12 480 (cả màn) | | Ghi vùng ảnh cũ (OLD) | G, H | tr.20 |
| `0x13` | DTM2 | 12 480 (cả màn) | | Ghi vùng ảnh mới (NEW) | G, H | tr.21 |
| `0x12` | DRF, Display Refresh | 0 | | Bắt đầu đảo màn theo RAM và bảng sóng; BUSY thấp | G, H | tr.20 |
| `0x50` | CDI | 1 | `0xD7` | Cực tính dữ liệu (DDX), viền (VBD), khoảng VCOM–dữ liệu | G, H | tr.29–30 |
| `0x90` | PTL, Partial Window | 7 | | Đặt cửa sổ partial | G | tr.37 |
| `0x91` | PTIN | 0 | | Vào chế độ partial | G | tr.37 |
| `0x92` | PTOUT | 0 | | Ra chế độ partial | G | tr.37 |
| `0xE0` | CCSET | 1 | `0x00` | Bit D1 = TSFIX: dùng nhiệt độ đặt tay thay cho cảm biến | G, H | tr.39 |
| `0xE5` | TSSET | 1 | `0x00` | Giá trị nhiệt độ đặt tay (mã = °C) | G, H | tr.40 |
| `0x71` | FLG, Get Status | đọc 1 byte | `0x13` | Đọc cờ trạng thái (xem dưới) | | tr.33 |
| `0x17` | AUTO | 1 (`0xA5` hoặc `0xA7`) | | Tự chạy PON, DRF, POF (và DSLP với `0xA7`) | | tr.21 |

## Bảng đầy đủ các lệnh còn lại

| Mã | Tên | Tham số (số byte) | Mặc định | Tác dụng | Datasheet |
|---|---|---|---|---|---|
| `0x01` | PWR | 5 | `03 10 3F 3F 0D` | Chọn nguồn trong hoặc ngoài; VGH/VGL; VSH, VSL, VDHR | tr.16–18 |
| `0x03` | PFS | 1 | `0x00` | Khoảng thời gian tắt source tới gate khi POF | tr.18 |
| `0x05` | PMES | 0 | | Bật bandgap, xoá ở POF kế | tr.19 |
| `0x06` | BTST | 3 | `17 17 17` | Soft-start tăng áp: thời gian, độ mạnh, thời gian tắt tối thiểu | tr.19 |
| `0x11` | DSP, Data Stop | 0; đọc cờ `data_flag` | | Báo hết dữ liệu; nếu `data_flag = 1` thì bắt đầu làm mới | tr.20 |
| `0x20` | LUTC | 56 (8 nhóm × 7) | | Bảng sóng VCOM | tr.22 |
| `0x21` | LUTWW | 42 (6 nhóm × 7) | | Bảng sóng trắng → trắng (chế độ KW) | tr.23 |
| `0x22` | LUTKW / LUTR | 56 hoặc 42 | | Bảng sóng đen → trắng (hoặc đỏ) | tr.24 |
| `0x23` | LUTWK / LUTW | 56 hoặc 42 | | Bảng sóng trắng → đen | tr.24 |
| `0x24` | LUTKK / LUTK | 56 hoặc 42 | | Bảng sóng đen → đen | tr.24 |
| `0x2A` | LUTOPT | 5 | | Tuỳ chọn bảng sóng: XON, nhóm bỏ qua | tr.25 |
| `0x30` | PLL | 1 | `0x06` | Tần số khung hình 5–200 Hz | tr.26 |
| `0x40` | TSC | đọc 2 | | Đọc nhiệt độ từ cảm biến trong | tr.27 |
| `0x41` | TSE | 1 | `0x00` | Chọn cảm biến trong hoặc ngoài; bù nhiệt độ TO[3:0] | tr.27 |
| `0x42`, `0x43` | TSW, TSR | 3 ghi, 2 đọc | | Ghi/đọc cảm biến ngoài LM75 qua I2C (không nối trên module) | tr.28 |
| `0x44` | PBC | đọc 1 | | Kiểm tra kính vỡ (ITO viền) | tr.28, 54 |
| `0x51` | LPD | đọc 1 | `0x01` | Cờ nguồn thấp (VDD dưới ngưỡng) | tr.30 |
| `0x60` | TCON | 1 | `0x22` | Thời gian chồng lấn gate/source (đơn vị 650 ns) | tr.31 |
| `0x61` | TRES | 3 | `0` | Đặt độ phân giải (HRES, VRES); ưu tiên hơn bit RES của PSR | tr.32 |
| `0x65` | GSST | 3 | `0` | Điểm bắt đầu gate/source | tr.32 |
| `0x70` | REV | đọc 5 | | Phiên bản chip (`0x09`) và LUT | tr.33 |
| `0x72` | CRC | đọc | | CRC dữ liệu | tr.33 |
| `0x80`, `0x81`, `0x82` | AMV, VV, VDCS | | | Đo và đặt VCOM | tr.34–36 |
| `0xA0`, `0xA1`, `0xA2` | PGM, APG, ROTP | | | Lập trình và đọc OTP | tr.37–38 |
| `0xE3` | PWS | 1 | `0x00` | Tiết kiệm năng lượng khi đảo cực VCOM/source | tr.39 |
| `0xE4` | LVSEL | 1 | `0x03` | Ngưỡng báo nguồn thấp (mặc định < 2,5 V) | tr.40 |

Mọi mã không có trong hai bảng là mã dành riêng, không được dùng (UC8253 tr.13 ghi chú 1).

## Giải mã thanh ghi quan trọng

### Lệnh `0x00` PSR (2 byte tham số)

| Bit | Byte 1 | Giá trị |
|---|---|---|
| D7, D6 | RES[1:0] | Số kênh hoạt động: 00 = 240 gate × 120 source; 01 = 320 × 160; 10 = 400 × 200; 11 = 480 gate × 240 source |
| D5 | REG | 0 = bảng sóng lấy từ OTP (mặc định); 1 = lấy từ thanh ghi (lệnh `0x20`–`0x24`) |
| D4 | KW/R | 0 = đen/trắng/đỏ (mặc định); 1 = chỉ đen/trắng (chế độ KW, dùng cho module này) |
| D3 | UD | 0 = quét gate xuống; 1 = quét lên (mặc định) |
| D2 | SHL | 0 = dịch source sang trái; 1 = sang phải (mặc định) |
| D1 | SHD_N | 0 = tắt tăng áp; 1 = bật (mặc định) |
| D0 | RST_N | 0 = reset mềm (BUSY thấp, lệnh khác bị bỏ qua ít nhất 50 µs); 1 = bình thường |

Byte 2: D4 VCMZ (VCOM nổi), D3 TS_AUTO (tự đo nhiệt độ một lần trước khi bật tăng áp, mặc định 1), D2 TIEG (sau POF nối VGL xuống đất, mặc định 1), D1 NORG (1 = ngoài lúc làm mới, VCOM nối đất), D0 VC_LUTZ (sau làm mới để VCOM nổi, mặc định 1).

Cột RES trong datasheet ghi thứ tự "(source x gate)" nhưng liệt kê kênh hoạt động theo dạng gate trước; bảng trên theo cột kênh hoạt động. Giá trị hãng dùng: `0xD7` (RES = 11, REG = 0, KW, quét xuống (UD = 0), dịch phải (SHL = 1), tăng áp bật), GxEPD2 dùng `0x1E` rồi `0x1F` (RES = 00; `0x1E` có RST_N = 0 nên là reset mềm).

### Lệnh `0x50` CDI (1 byte)

| Bit | Trường | Ý nghĩa |
|---|---|---|
| D7, D6 | VBD[1:0] | Chọn dữ liệu cho viền (border): nổi hoặc theo một bảng sóng; xem UC8253 tr.29 |
| D5, D4 | DDX[1:0] | Cực tính dữ liệu; xem `image-memory.md` |
| D3–D0 | CDI[3:0] | Khoảng giữa VCOM và dữ liệu: 0000 = 17 hsync, 0111 = 10 (mặc định), 1111 = 2 |

Ví dụ giải mã: `0xD7` = VBD 11, DDX 01, CDI 0111 (mặc định); `0x97` = VBD 10, DDX 01, CDI 0111; `0x47` = VBD 01, DDX 00, CDI 0111; `0xC7` = VBD 11, DDX 00, CDI 0111.

### Lệnh `0x71` FLG (đọc 1 byte)

| Bit | Cờ | Ý nghĩa |
|---|---|---|
| D6 | PTL_FLAG | 1 = đang ở chế độ partial |
| D5 | I2C_ERR | Lỗi I2C master |
| D4 | I2C_BUSYN | I2C bận (tích cực thấp) |
| D3 | data_flag | 1 = đã nhận đủ một khung dữ liệu |
| D2 | PON | Đang bật nguồn |
| D1 | POF | Đã tắt nguồn |
| D0 | BUSY_N | 0 = bận |

Giá trị sau reset là `0x13`.

Đối chiếu gốc: `source/uc8253-datasheet.pdf` tr.8–13 (bảng lệnh), tr.14–40 (mô tả từng lệnh). Bảng lệnh trong PDF trích text bị bể cột; mở trang PDF để chắc chắn.
