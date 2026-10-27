# Module e-paper WeAct 3.7": cơ khí và lắp ráp

Kích thước bo module, kích thước panel, đầu nối và các lưu ý khi lắp để không làm hỏng màn.

## Ví dụ

Để làm hộp, cần biết hai thứ: bo module là 105,79 × 53,80 mm với bốn lỗ Ø3,2 mm cho vít M3, và vùng nhìn thấy của màn là 47,04 × 81,54 mm.

## Bo module

| Hạng mục | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Kích thước bo | 105,79 × 53,80 mm | Bản vẽ hình dạng | Đã xác nhận |
| Lỗ bắt vít | 4 lỗ Ø3,2 mm cho vít M3, ở bốn góc | Bản vẽ hình dạng; sơ đồ (X1–X4) | Đã xác nhận |
| Bán kính góc bo | R1,4 mm | Bản vẽ hình dạng | Đã xác nhận |
| Các kích thước còn lại trên bản vẽ | 92,99; 48,20; 8,30; 2,80; 1,93 mm | Bản vẽ hình dạng | Đã xác nhận (có số; ý nghĩa từng số xem bản vẽ) |
| Đầu 8 chân | Hàng 2 × 4 bước 2,54 mm và đầu Wafer 1,25 mm 8 chân, đặt gần góc trên phải | Bản vẽ hình dạng | Đã xác nhận |

Bản vẽ không ghi khoảng cách tâm hai lỗ theo chiều dài; đo trên hình thì khoảng 100 mm (ước lượng, không dùng để gia công). Số 48,20 mm có vẻ là khoảng cách hai lỗ theo chiều rộng; cũng nên xác minh trên bản vẽ.

## Panel

| Hạng mục | Giá trị | Nguồn |
|---|---|---|
| Kích thước ngoài | 53 (rộng) × 92,99 (cao) × 0,95 (dày) mm, chưa tính lớp phim che | Panel tr.5 |
| Độ dày tổng có FPC, không phim bảo vệ | 0,90 ± 0,10 mm | Panel tr.6 |
| Vùng hiển thị | 47,04 × 81,54 mm | Panel tr.5 |
| Khối lượng | 8,5 ± 0,2 g | Panel tr.5 |
| FPC | 24 chân, bước 0,5 mm; đầu nối J1 `FPC0.5-SMT-24P` | Panel tr.6; sơ đồ |

## Đầu nối Wafer 1,25 mm (J2)

| Thông số | Giá trị | Nguồn |
|---|---|---|
| Bước chân | 1,25 mm, 8 chân, hàn bề mặt | Connector tr.1 |
| Điện áp / dòng định mức | 125 V AC/DC, 1 A | Connector tr.1 |
| Điện trở tiếp xúc | Tối đa 50 mΩ | Connector tr.1 |
| Nhiệt độ hoạt động | −25–70 °C | Connector tr.1 |

## Lưu ý khi lắp (từ tài liệu hãng)

- Gắn module phẳng hoặc trên đế cứng; không để chịu lực xoắn hoặc uốn liên tục. Panel là kính nên có thể vỡ.
- Nên đặt tấm bảo vệ trong suốt phía trước màn, đủ cứng để chịu lực ngoài.
- Không chạm, miết bề mặt phim phân cực bằng vật cứng hơn ruột chì HB; không dùng aceton, toluen, cồn để lau.
- Không dùng vỏ làm bằng vật liệu loại axit axetic hoặc clo (ăn mòn linh kiện).
- Chỉ dùng IPA trên vùng hiển thị và mặt sau kính; không tháo panel.
- Màn nhạy với tĩnh điện.
- Nếu vỡ, không chạm vào chất rò ra; dính thì rửa bằng xà phòng và nước.

Bản mô hình 3D `WeAct-EpaperModule-3.7 Board 3D.step` (khoảng 3,5 MB) không copy vào repo; nằm trong thư mục gốc (`sources.md`).

Đối chiếu gốc: `source/module-board-shape-3.7.pdf`; `source/panel-e037a75-a0-spec.pdf` tr.5–6, 14–15; `source/connector-wafer-125w-8p-datasheet.pdf` (1 trang).
