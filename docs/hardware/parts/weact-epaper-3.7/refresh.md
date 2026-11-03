# Module e-paper WeAct 3.7": làm mới màn

Panel GDEY037T03 (xem `overview.md`), controller UC8253. File này ghi thời gian làm mới đã đo và cách chống bóng mờ.

Cột "Trạng thái": `Đã xác nhận` (đã đo trên máy) hoặc `Chưa xác nhận`.

## Ví dụ

Vẽ một ảnh mới đầy màn bằng sóng chuẩn mất khoảng 2,8 giây, màn chớp đen trắng vài lần. Cập nhật số đếm ở một ô nhỏ bằng partial refresh mất khoảng 0,42 giây và không chớp cả màn; làm nhiều lần thì hình cũ để lại bóng mờ, nên cứ vài chục lần cần một lần làm sạch.

## Thời gian


| Kiểu làm mới                                                                                               | Thời gian                                                                                                                     | Nguồn                                                                   | Trạng thái    |
| --------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- | ----------------- |
| Full refresh (gồm truyền dữ liệu)                                                                         | Sóng chuẩn 2,8 s; sóng nhanh 2,3 s. Thư viện ghi ~1,0 s cho riêng phần panel quét                                      | Đo bằng demo, 2026-10-21                                               | Đã xác nhận |
| Full refresh bằng `ink_drv_uc8253` (sóng OTP, ép nhiệt độ 90 như GxEPD2, SPI 4 MHz)                    | Gửi ảnh 25 ms; PON + DRF tới khi BUSY lên cao 2 137 ms; POF + ghi lại ảnh cũ 65 ms                                      | Đo trên BB0, 2026-10-28, một lần, lần vẽ đầu sau khi cấp điện | Đã xác nhận |
| Deep clean                                                                                                    | 8,3 s (1 vòng đen → trắng, rồi vẽ)                                                                                       | Đo bằng demo                                                           | Đã xác nhận |
| Partial refresh bằng `ink_drv_uc8253` (ép nhiệt độ 110 như GxEPD2, SPI 4 MHz), một khung chữ 80 × 40 | 422 ms từ lúc gọi vẽ tới khi BUSY lên cao: gửi ảnh 1 ms, PON 75 ms, DRF 344 ms; sau đó POF + ghi lại ảnh cũ 41 ms | Đo trên BB0, 2026-10-28, 10 lần (lệch nhau dưới 2 ms)              | Đã xác nhận |
| Partial refresh như trên, vùng cả màn 240 × 416                                                         | Gửi ảnh 25 ms, panel 420 ms; sau đó 65 ms                                                                                  | Như trên                                                               | Đã xác nhận |

Tài liệu hãng (panel và datasheet UC8253) không ghi thời gian làm mới. Thời gian DRF bằng (số khung của bảng sóng dài nhất + 2) chia cho tần số khung của dải nhiệt độ đang dùng; số khung và tần số khung đã đọc từ OTP và khớp số đo (`waveform.md`).

## Các biến thể partial refresh

Đo trên BB0 bằng `ink_drv_uc8253`, 2026-10-28, mỗi biến thể 10 lần, số là trung bình. Trình tự: ghi ảnh mới vào `0x13` trong window; rồi `0x91`, `0x90` (window), lệnh nhiệt độ, `50 D7`, PON, DRF, `0x92`; rồi POF và ghi lại window vào `0x10`. Cột "Gọi → BUSY" tính từ lúc gọi hàm vẽ tới khi BUSY lên cao sau DRF.


| Vùng      | `PT_SCAN` | Nhiệt độ | Gọi → BUSY | Gửi ảnh | Panel (PON + DRF) | Sau khi quét |
| ------------ | ----------- | ------------- | -------------- | ----------- | ------------------- | --------------- |
| 80 × 40   | 1         | ép 110     | 422,7 ms     | 1,0 ms    | 421,1 ms          | 41,2 ms       |
| 80 × 40   | 0         | ép 110     | 422,5 ms     | 1,0 ms    | 420,9 ms          | 41,2 ms       |
| 80 × 40   | 1         | cảm biến  | 2 615,8 ms   | 1,0 ms    | 2 614,2 ms        | 41,2 ms       |
| 80 × 40   | 0         | cảm biến  | 2 615,3 ms   | 1,0 ms    | 2 613,7 ms        | 41,2 ms       |
| 240 × 416 | 1         | ép 110     | 445,6 ms     | 25,3 ms   | 420,3 ms          | 65,4 ms       |
| 240 × 416 | 0         | ép 110     | 445,7 ms     | 25,3 ms   | 420,4 ms          | 65,4 ms       |
| 240 × 416 | 1         | cảm biến  | 2 639,2 ms   | 25,3 ms   | 2 613,9 ms        | 65,4 ms       |
| 240 × 416 | 0         | cảm biến  | 2 650,7 ms   | 25,3 ms   | 2 625,4 ms        | 65,4 ms       |

Thời gian DRF theo nhiệt độ ép (vùng 80 × 40, mỗi giá trị 3 lần; PON luôn khoảng 75 ms):


| Nhiệt độ ép (`E5`) | DRF      |
| ------------------------ | ---------- |
| 110, 120, 127          | 344 ms   |
| 100                    | 1 492 ms |
| 90                     | 2 055 ms |
| 50, 70                 | 2 598 ms |
| 30                     | 2 538 ms |

Kết luận từ số đo:

- `PT_SCAN` không đổi thời gian.
- Phải ép nhiệt độ. Không ép thì controller đo nhiệt độ phòng và chạy bảng sóng dài khoảng 2,6 s. Muốn không ép thì phải gửi `E0 00`, vì bit TSFIX do full refresh bật vẫn còn trong thanh ghi.
- Ép từ 110 trở lên chọn dải nhiệt độ nóng nhất trong OTP (TR8), DRF 344 ms. Không dải OTP nào cho DRF ngắn hơn, nên với bảng sóng OTP thì một lần partial không xuống dưới khoảng 420 ms (PON + DRF), hoặc 344 ms nếu giữ nguồn bật. Bảng sóng nạp qua thanh ghi thì ngắn hơn được (phần "Bảng sóng nạp qua thanh ghi").
- Gửi ảnh qua SPI chỉ đáng kể khi vùng lớn: 1 ms cho 80 × 40, 25 ms cho cả màn.
- Đo từ cạnh bấm nút (ISR) tới khi BUSY lên cao, chưa qua nhận dạng cử chỉ, ép 110: vùng 80 × 40 mất 423–425 ms (6 lần); cả màn mất 462–469 ms (58 lần), trong đó khoảng 18 ms là chương trình đo tô lại cả khung.

Giữ nguồn analog bật giữa các lần partial (không POF sau khi quét, lần sau không cần PON, như GxEPD2), ép 110, `PT_SCAN` = 1. Đo trên BB0, 2026-10-28:


| Vùng      | Số lần | Gọi → BUSY                                                                                | Gửi ảnh | Panel    | Sau khi quét |
| ------------ | ---------- | --------------------------------------------------------------------------------------------- | ----------- | ---------- | --------------- |
| 80 × 40   | 50       | 346 ms từ lần thứ hai (lần đầu 420 ms vì còn PON); trung bình cả 50 lần 347,7 ms | 1,0 ms    | 344,7 ms | 1,0 ms        |
| 240 × 416 | 10       | 384,8–389,2 ms                                                                             | 25,3 ms   | 344,7 ms | 25,2 ms       |

Cách này bỏ được PON (75 ms) và POF (40 ms), nhưng DRF 344 ms vẫn còn, nên vẫn trên 0,3 s. Bóng mờ sau 50 lần với cách này chưa ghi nhận. Driver có tuỳ chọn `keep_power_on`, mặc định tắt.

Biến thể nhanh nhất mà không để lại bóng mờ rõ: ép 110, `PT_SCAN` = 1, đây là mặc định của driver.

## Bảng sóng nạp qua thanh ghi

Bảng TR8 đọc từ OTP, nạp lại qua thanh ghi (REG = 1, `waveform.md`), cắt bớt khung hoặc đổi tần số khung. Đo trên BB0, 2026-10-30, ô 80 × 40, giữ nguồn analog bật, mỗi dòng 5–10 lần, lệch nhau dưới 0,1 ms.


| Bảng sóng                        | Panel (DRF)                                  | (N + 2) × chu kỳ khung |
| ------------------------------------ | ---------------------------------------------- | -------------------------- |
| TR8 từ OTP, REG = 0               | 344,6 ms                                     | 17 × 20 = 340 ms        |
| TR8 qua thanh ghi, 15 khung, 50 Hz | 343,1 ms                                     | 340 ms                   |
| 12 khung, 50 Hz                    | 283,1 ms                                     | 280 ms                   |
| 10 khung, 50 Hz                    | 242,9 ms                                     | 240 ms                   |
| 6 khung, 50 Hz                     | 162,8 ms                                     | 160 ms                   |
| 4 khung, 50 Hz                     | 122,7 ms                                     | 120 ms                   |
| 2 khung, 50 Hz                     | 82,6 ms                                      | 80 ms                    |
| 1 khung, 50 Hz                     | 62,6 ms                                      | 60 ms                    |
| 15 khung, 65 Hz                    | 266,1 ms                                     | 262 ms                   |
| 15 khung, 75 Hz                    | 227,5 ms                                     | 227 ms                   |
| 15 khung, 100 Hz                   | 171,8 ms                                     | 170 ms                   |
| 15 khung, 150 Hz                   | Khoảng 113 ms (đo cả PON rồi trừ 75 ms) | 113 ms                   |
| 15 khung, 200 Hz                   | 86,1 ms                                      | 85 ms                    |
| 4 khung, 200 Hz                    | 31,0 ms                                      | 30 ms                    |

Thời gian DRF chỉ phụ thuộc số khung và tần số khung, không phụ thuộc cỡ vùng. POF mất khoảng 2 khung: 44 ms ở 50 Hz, 24 ms ở 100 Hz, 18 ms ở 150 Hz.

## Bóng mờ theo bảng sóng

Bài đo: full refresh màn trắng hai lần; mỗi dải dọc rộng 40 điểm đảo N lần giữa ô cờ 8 × 8 điểm và ô cờ ngược, rồi về trắng, bằng một bảng sóng; nhìn bằng mắt trên BB0, 2026-10-30. Dải tham chiếu không đảo lần nào, chỉ có màu trắng của full refresh.


| Bảng sóng                                      | Panel mỗi lần đổi        | N = 20         | N = 100                                   |
| -------------------------------------------------- | ------------------------------ | ---------------- | ------------------------------------------- |
| TR8: 15 khung, 50 Hz                             | 344 ms                       | Sạch          | Sạch, trắng hơn dải tham chiếu       |
| 12 khung, 50 Hz                                  | 283 ms                       |                | Xám rất nhẹ, gần như TR8             |
| 15 khung, 65 Hz                                  | 266 ms                       |                | Sạch, như TR8                           |
| 10 khung, 50 Hz                                  | 243 ms                       |                | Xám nhẹ, không thấy ô cờ            |
| 15 khung, 75 Hz                                  | 228 ms                       |                | Xám nhẹ hơn 10 khung một chút        |
| 15 khung, 100 Hz                                 | 172 ms                       | Xám rất nhẹ | Thấy bóng ô cờ khi soi kỹ            |
| 6 khung, 50 Hz                                   | 163 ms                       | Xám rất nhẹ | Xám rõ hơn 100 Hz, không thấy ô cờ |
| TR8, `LUTKK` toàn 0 V                           | 343 ms                       |                | Xám rất nhẹ, không thấy ô cờ       |
| 3 lần quét 5 khung, 50 Hz, cho mỗi lần đổi | Khoảng 430 ms (3 × 143 ms) | Sạch          | Sạch, như TR8                           |

- Một lần full refresh (ép 90) sau N = 100 xoá sạch mọi vết.
- Hai cách hỏng khác nhau: kéo đủ số khung nhưng mỗi khung ngắn (100 Hz) thì còn lại hình cũ; ít khung (6 khung) thì xám đều, không nhớ hình cũ.
- Mép giữa vùng được vẽ partial nhiều lần và vùng bên cạnh để lại một vệt xám rất nhẹ, chỉ thấy khi soi kỹ (15 khung, 65 Hz, N = 100).
- Dải TR8 và dải `LUTKK` toàn 0 V lẽ ra như nhau (mọi điểm đều đổi màu ở mỗi lần đảo nên `LUTKK` không được dùng), nhưng nhìn lệch nhau một chút: mức "xám rất nhẹ" nằm trong độ lệch của phép so bằng mắt và độ đều của panel.

## Chống bóng mờ và chấm lốm đốm

- 50 lần partial liên tiếp (ép 110, `PT_SCAN` = 1) trong ô 80 × 40, nội dung là 10 cột đen trắng đổi theo số đếm: không thấy bóng mờ. Ở 1–2 lần cuối, cột đổi màu nhiều nhất (đảo màu mỗi lần vẽ) hiện màu xám (nhìn bằng mắt trên BB0, 2026-10-28). Bài này chạy với trình tự driver cũ, vốn không vẽ ở mọi lần DRF thứ chẵn (`image-memory.md`), nên chỉ khoảng một nửa số lần là vẽ thật; số liệu bóng mờ dùng bảng ở phần trên.

Kết quả từ bản demo:

- Full refresh đầu tiên sau khi cấp điện bằng sóng nhanh (ép nhiệt độ 90) còn để lại bóng ảnh cũ (đo trên BB0 bằng `ink_drv_uc8253`, 2026-10-28; lúc đó vùng ảnh cũ trong controller chưa được ghi).
- Sóng chuẩn cho full refresh sạch hơn sóng nhanh (bỏ lệnh ép nhiệt độ `E0 02` / `E5 5A`), chậm hơn khoảng 0,5 s.
- Deep clean: đảo cả màn đen rồi trắng trước khi vẽ. Mỗi vòng đen → trắng thêm khoảng 5,5 s. Bản demo xác nhận cách này xoá được chấm lốm đốm ở góc phải.
- Partial refresh nhiều lần sẽ tích bóng mờ; cứ vài chục lần nên deep clean một lần.

## Đo bằng display_bench, 2026-11-04

Firmware test bản `9e5bf0a` (bảng sóng luôn nạp qua thanh ghi, REG = 1), 30 °C theo cảm biến, nhìn bằng mắt trên BB0.
Kết quả thô: `tools/display_bench/results/2026-11-04.jsonl`.

| Bài | Cấu hình | Kết quả |
|---|---|---|
| Ô 80 × 40 lật 10 lần cùng chỗ | `lut10f50`, window vùng đổi, panel 250 ms | Ô vừa về trắng còn xám rất mờ (xám trong vùng); viền quanh ô 3/5 |
| Ô cờ 16 điểm đảo 100 lần cả màn, rồi về trắng | TR8 (`tr8`), chế độ partial, panel 344 ms | Trong lúc đảo không thấy bóng; sau khi về trắng còn đốm của ô đen, màn xám dần. Chưa chạy với `tr8_kk0` để so |
| Trang chữ cuộn | TR8, window vùng đổi, panel 352 ms | Chữ ổn (4/5) |
| Ô A, rồi vẽ ô B ở chỗ khác | TR8, `PT_SCAN` = 0, window quanh B | Ô A không đổi. `PT_SCAN` = 1 chưa chạy |
| Cả màn đổi trắng ↔ đen | TR8, chế độ partial | Không kéo viền (CDI `0xD7`) thì viền màn xám; kéo viền (`0x97`) thì đều màu, kể cả trong chế độ partial |
| Ô lật 10 lần | Nhiều profile (có `3x5f50`), kéo viền | Viền đen xám hai bên ô ở mọi profile |
| Dải xám dither, xem ngang | TR4 (`tr4`) cả màn ngoài chế độ partial, 2 598 ms | Vùng đen của dải trên cùng nhạt dần về phía vùng trắng; xem dọc thì đen đều. Chưa rõ nguyên nhân |
| Cả màn đổi trắng ↔ đen 6 lần | `tr4` | Đổi ổn; sau nhiều lần đổi, màn tự xám dần khi nghỉ |
| Cảm biến nhiệt `0x40` khi REG = 1 | 10 lần đọc, mỗi giây một lần | Luôn 30 °C, lệch nhiệt độ phòng khoảng 2–3 °C |
| Bảng dọn 200 Hz cân bằng (`clean_full`, `clean_edge`, `clean_light`) | Thang dọn của `ink_display` | DRF khớp số tính (`clean_edge` 51 ms); PON khi nguồn đã tắt thêm khoảng 70 ms. Vùng trắng được dọn đục dần |
| Dọn mạnh khi vượt trần nợ | `tr4` ở 30 °C | 2 553 ms mỗi lần, 3 lần trong 20 lần lật với ý định `UI` |
| Đổi profile dọn tâm viền, sau lần đo trên | `clean_full50`, `clean_full100` thay `clean_full`; dọn quanh viền `clean_edge`, dọn nhẹ `clean_light` | Dọn tâm dài làm viền khó thấy hơn; `clean_full100` ổn nhất |
| Nghỉ qua dọn nhẹ rồi tắt nguồn | PSR byte 2 `0x0D`, nhiều profile cho dọn nhẹ | Có lúc vài hạt khắp màn tự xám hẳn sau khi tắt nguồn, profile nào cũng gặp. Chưa rõ nguyên nhân |
| Bài "PSR byte 2 và tắt nguồn" | PSR byte 2 `0x0D` (mặc định của UC8253) so với `0x0E` (mã mẫu hãng) | Không khác nhau |
| Nghỉ qua dọn nhẹ rồi tắt nguồn, chạy nhiều lần | Như trên | Hạt xám lúc có lúc không, không lặp lại được |
| Ô cờ đảo nhiều lần rồi về trắng | `tr8_kk0` (bảng đen giữ đen 0 V) | Vẫn xám như `tr8`: bảng KK không phải nguyên nhân |
| Ô A, rồi vẽ ô B ở chỗ khác | TR8, `PT_SCAN` = 1 | Ô A không đổi |
| Dọn nhanh `tr8` khi vượt trần nợ (cả màn, mặt phẳng cũ đảo bit) | Thử thay dọn mạnh | Vùng không đổi đậm dần sau mỗi lần dọn, nặng hơn nhiều so với hạt xám ở trên |
| Bài "VCOM và vùng đứng yên" | `tr8`, VCOM_DC `0x13` (−2,0 V) so với `0x09` (−1,0 V) | −2,0 V tệ hơn. −1,0 V: trong lúc chạy ổn; sau khi chạy, nửa trắng đứng yên tối dần, nửa đen giữ nguyên |
| Bài "VCOM và vùng đứng yên", quét VCOM_DC thấp hơn `0x09` | `tr8` | Nửa trắng vẫn tối dần; đen không đủ, nhạt và chớp. Nửa trắng tối đi đúng lúc tắt nguồn (log "tắt nguồn") |
| Trang chữ cuộn, so ứng viên | `tr8` | `tr8` đủ tốt cho chữ |
| Đẩy bù thử tay | `lut10f50` rồi `push5f50`, mặt phẳng cũ là slot trắng | Ô được đẩy sạch, không chớp, nhưng đậm hơn vùng xung quanh. Lần thử này đẩy mọi điểm đen trong ô, kể cả điểm đã đen đủ |

## Khuyến nghị của hãng panel


| Khuyến nghị                                                                                              | Nguồn      |
| ------------------------------------------------------------------------------------------------------------ | ------------- |
| Làm mới ít nhất một lần mỗi 24 giờ; trên 35 °C hiệu ứng điểm đơn không được đảm bảo | Panel tr.11 |
| Độ ẩm 45–70 %RH để giữ chất lượng 5 năm; nên làm mới mỗi ngày                              | Panel tr.12 |
| Bóng mờ (ΔE) tối đa 2                                                                                 | Panel tr.12 |

Đối chiếu gốc: `source/panel-e037a75-a0-spec.pdf` tr.11–12; `source/uc8253-datasheet.pdf` tr.22–26, 43–45.
