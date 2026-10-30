# Module e-paper WeAct 3.7": thứ tự gửi lệnh

Ba trình tự đã biết: của mã mẫu hãng, của thư viện GxEPD2 (đã chạy trên máy qua demo), và các quy tắc nguồn trong datasheet. Từng lệnh giải thích ở `commands.md`; chi tiết bộ nhớ ảnh ở `image-memory.md`.

## Ví dụ: vẽ một ảnh rồi cho màn ngủ

Reset phần cứng, chờ BUSY cao, gửi cấu hình panel và cực tính dữ liệu, đẩy 12 480 byte ảnh, bật nguồn, làm mới, chờ BUSY cao, tắt nguồn, cho ngủ sâu. Màn giữ ảnh, ESP32 và controller đều có thể nghỉ. Muốn vẽ ảnh khác thì làm lại từ bước reset.

## Quy tắc chung từ datasheet

| Quy tắc | Nguồn |
|---|---|
| Sau khi RST_N lên cao, chờ hơn 1 ms rồi mới gửi lệnh | UC8253 tr.43 |
| PON (`0x04`): đo nhiệt độ một lần rồi bật tăng áp; BUSY thấp cho tới khi điện áp sẵn sàng | UC8253 tr.19, 43 |
| Sau khi làm mới, VCOM tự chuyển sang nổi | UC8253 tr.43 |
| Khi bảng sóng lấy từ OTP, bảng được chép sang thanh ghi tự động sau DSP hoặc DRF | UC8253 tr.43 |
| POF (`0x02`) tắt tăng áp nhưng giữ thanh ghi cho tới khi mất nguồn VDD hoặc vào deep sleep | UC8253 tr.18 |
| Deep sleep (`0x07`, `0xA5`) chỉ thoát bằng reset phần cứng | UC8253 tr.20, 53 |
| Lệnh `0x17` AUTO với `0xA5` chạy sẵn PON, DRF, POF; với `0xA7` chạy thêm DSLP | UC8253 tr.21 |
| Trong lúc làm mới, hầu hết lệnh bị bỏ qua (xem `interface.md`) | UC8253 tr.46 |

## Luồng hoạt động chuẩn của hãng

Tài liệu panel GDEY037T03 tr.39 ("Normal Operation Flow"):

| Bước | Lệnh |
|---|---|
| Reset IC | RST_N thấp hơn 50 µs, cao, chờ hơn 1 ms |
| Initialize | PSR, PWR, PFS, BTST, PLL, CDI, TCON, VDCS, PWS |
| Display | TRES, GSST, DTM1 (ảnh), DTM2 (ảnh), PON, chờ BUSY, DRF, chờ BUSY, POF, chờ BUSY |
| Deep sleep | DSLP |

Luồng này không có partial refresh. Mỗi lần cập nhật đi lại từ reset và ghi đủ hai vùng RAM.

## Trình tự của mã mẫu hãng (đen trắng)

Nguồn: `source/panel-vendor-init-uc8253.c`. Chạy trên MSP430, nhưng phần gọi lệnh dùng được cho mọi MCU.

Khởi tạo (`INIT_UC8253`, dòng 354–362), chạy sau mỗi lần reset:

| Bước | Lệnh | Tham số | Ý nghĩa |
|---|---|---|---|
| 1 | Reset phần cứng | cao 1 ms, thấp 5 ms, cao 1 ms | |
| 2 | Chờ BUSY cao | | |
| 3 | `0x00` | `D7 0E` | KW, 480 gate × 240 source, bảng sóng từ OTP, tăng áp bật; byte 2: đo nhiệt độ tự động, nối VGL xuống đất sau POF, ngoài lúc làm mới thì nối VCOM xuống đất |
| 4 | `0x50` | `47` | DDX = 00 (bit 1 là đen), VBD = 01 |

Vẽ kiểu chậm (`dis_img`, dòng 287–348):

| Bước | Lệnh | Ghi chú |
|---|---|---|
| 1 | `0x13` + 12 480 byte | Chỉ ghi ảnh mới |
| 2 | `0xE0` `02`; `0xE5` `53` | Ép nhiệt độ 83 (xem `waveform.md`), chú thích hãng: "slow update" |
| 3 | `0x04`, chờ BUSY | PON |
| 4 | `0x12`, chờ BUSY | DRF; hãng đệm 20 ms trước và sau lệnh |
| 5 | `0x02`, chờ BUSY | POF |
| 6 | `0x07` `A5` | Ngủ sâu |

Vẽ kiểu nhanh (`dis_img_fast`, dòng 176–283):

| Bước | Lệnh | Ghi chú |
|---|---|---|
| 1 | `0x50` `C7` | VBD = 11, DDX = 00 |
| 2 | `0x10` + 12 480 byte | Ghi ảnh cũ |
| 3 | `0x13` + 12 480 byte | Ghi ảnh mới |
| 4 | `0x04`, `0x12`, `0x02` (mỗi lệnh chờ BUSY) | Không có `E0`/`E5`: dùng nhiệt độ đo thật |
| 5 | `0x07` `A5` | Ngủ sâu |

Chương trình chính của hãng lặp reset → khởi tạo → vẽ → ngủ cho mỗi ảnh. Mỗi ảnh đều đi lại từ bước reset, kể cả kiểu nhanh.

## Trình tự của GxEPD2 và demo

Nguồn: `src/gdey/GxEPD2_370_GDEY037T03.cpp` và `src/GxEPD2_BW.h` (bản master, đọc 2026-10-30), đã chạy trên máy qua demo; cột "Giải mã" là giải mã theo datasheet.

| Bước | Lệnh | Giải mã |
|---|---|---|
| Khởi tạo | `0x00` `1E 0D`, rồi `0x00` `1F 0D` | PSR: RES = 00, KW, quét lên, dịch phải; `1E` có RST_N = 0 nên là reset mềm, `1F` có RST_N = 1 |
| Ghi vùng ảnh | `0x91`, `0x90` + cửa sổ (x, xe, y, ye, `01`), `0x13` hoặc `0x10` + dữ liệu, `0x92` | PTIN, PTL, DTM2 hoặc DTM1, PTOUT |
| Full refresh | `E0 02`, `E5 5A`, `50 97`, `04`, `12`, `02` | Ép nhiệt độ 90; CDI: VBD = 10, DDX = 01; PON; DRF; POF |
| Partial refresh | `E0 02`, `E5 6E`, `50 D7`, `04`, `12`, `02` | Ép nhiệt độ 110; CDI mặc định; PON; DRF; POF |
| Sau mỗi lần làm mới, cả full lẫn partial | `0x00` `1E 0D`, rồi `0x00` `1F 0D` (hàm `_InitDisplay`, chú thích "undo TSFIX") | Reset mềm |
| Ngủ sâu | `07` `A5` | DSLP |

Hàm `display()` của GxEPD2 ghi **cả màn**: `0x13` trước khi làm mới, `0x10` sau khi làm mới (`writeImageAgain`), kể cả khi làm mới partial. Ghi cả màn ở mỗi lần vẽ là điều kiện để bước reset mềm không gây hại: reset mềm làm RAM ảnh thành rác (`image-memory.md`), và lần ghi cả màn tiếp theo đè hết rác.

Sóng "chuẩn" của GxEPD2 (full refresh sạch hơn) bỏ lệnh ép nhiệt độ `E0 02` / `E5 5A`; sóng "nhanh" giữ lệnh đó (xem `refresh.md`).

## Trình tự của `ink_drv_uc8253`

Đã chạy trên BB0 (2026-10-28); thời gian đo ở `refresh.md`. Khởi tạo như GxEPD2. Controller không ngủ sâu giữa các lần vẽ.

| Bước | Full refresh | Partial refresh |
|---|---|---|
| 1. Ghi ảnh mới | `0x91`, `0x90` (cả màn), `0x13` + 12 480 byte, `0x92` | `0x91`, `0x90` (window, mép x căn theo bội 8), `0x13` + dữ liệu window, `0x92` |
| 2. Làm mới | `E0 02`, `E5 5A`, `50 97`, PON, DRF | `0x91`, `0x90` (window), `E0 02`, `E5 6E`, `50 D7`, PON, DRF, `0x92` |
| 3. Sau khi quét | POF, rồi ghi lại cả màn vào `0x10` | POF, rồi ghi lại window vào `0x10` |

- Driver chờ BUSY cao sau lệnh `0x90`. Bảng BUSY_N ở UC8253 tr.46 ghi PTL là "No action", nên bước chờ này không cần.
- PON gửi khi nguồn analog đang tắt. Nếu giữ nguồn bật giữa các lần partial (tuỳ chọn của driver, mặc định tắt) thì bỏ POF ở bước 3 và PON ở lần sau.
- Không ép nhiệt độ thì phải gửi `E0 00`: bit TSFIX do lần vẽ trước bật vẫn còn trong thanh ghi.
- **Lỗi đã đo:** trình tự này chỉ ghi ảnh mới bằng `0x13` trước DRF và ghi lại `0x10` sau DRF, không reset giữa các lần vẽ. Do hai vùng RAM đổi vai sau mỗi DRF, mọi lần DRF thứ chẵn kể từ reset không đổi hình, trong khi thời gian đo vẫn bình thường (BB0, 2026-10-30; `image-memory.md`).

## Trình tự partial đã chạy đúng trên BB0

Đo 2026-10-30, mã ở nhánh `exp/uc8253-lut` (commit `656de66`).

| Bước | Lệnh |
|---|---|
| 1. Ghi ảnh mới | `0x91`, `0x90` (window), lệnh ảnh mới theo lượt (`0x13` khi số DRF từ reset là chẵn, `0x10` khi lẻ) + dữ liệu window, `0x92` |
| 2. Ghi ảnh cũ | `0x91`, `0x90` (window), lệnh còn lại + ảnh đang hiện trong window, `0x92` |
| 3. Làm mới | `0x91`, `0x90` (window), `E0 02`, `E5 6E`, `50 D7`, PON (nếu đang tắt), DRF, `0x92` |
| 4. Sau khi quét | Tăng số đếm DRF; POF nếu không giữ nguồn. Không ghi gì thêm |

Kết quả: 5 dải vẽ liên tiếp và 7 lần lật liên tiếp đều đúng. SPI gấp đôi (ô 80 × 40: 2 ms), bước sau khi quét còn 0,01 ms khi giữ nguồn.

## So sánh hai trình tự

| Hạng mục | Mã mẫu hãng | GxEPD2 |
|---|---|---|
| Cực tính dữ liệu (bit DDX) | 00: bit 1 là đen | 01: bit 1 là trắng |
| Hướng quét gate (bit UD) | 0 (xuống) | 1 (lên) |
| Gửi `0x61` (độ phân giải) | Không | Không thấy trong trình tự đã ghi |
| Cho ngủ sau mỗi ảnh | Có | Có (hàm `hibernate` của demo) |
| Ghi hai vùng RAM | Cả màn `0x10` và `0x13` trước DRF | Cả màn `0x13` trước DRF, cả màn `0x10` sau DRF |
| Reset giữa các lần vẽ | Reset cứng trước mỗi ảnh | Reset mềm sau mỗi lần làm mới |

Mã dùng từ hai nguồn trên phải khớp bit DDX của lệnh `0x50` và bit UD của lệnh `0x00` với dữ liệu ảnh đang đẩy; nếu lệch thì ảnh bị đảo màu hoặc đảo hướng.

Đối chiếu gốc: `source/panel-vendor-init-uc8253.c` dòng 78–98, 176–434; `source/uc8253-datasheet.pdf` tr.14, 18–21, 39–40, 43, 46; `source/gdey037t03-spec.pdf` tr.39; `source/example-arduino-esp32s3.ino` (dùng GxEPD2, dòng 29–30, 38, 47).
