# Canvas và chữ

## Vấn đề

App nghĩ về màn ngang 416 × 240, nhưng panel lưu ảnh dọc 240 × 416 và `display` đưa nguyên vùng nhớ ảnh cho driver.
Nếu app vẽ theo màn ngang rồi xoay cả khung sang bố cục panel thì tốn thêm 12 480 byte RAM và thời gian ở đoạn 2 của
đường vẽ (phải nhỏ hơn 50 ms, xem `display-pipeline.md`). Chữ phải có tiếng Việt có dấu, mà máy không có font hệ thống.

## Cách xử lý

Khối `ui` là C thuần. Nó không tự cấp phát bộ nhớ: bên gọi đưa vùng nhớ khung vào lúc khởi tạo.

### Canvas

Canvas lưu ảnh đúng bố cục vùng RAM ảnh của panel, nên `display` đưa thẳng cho driver, không chép hay xoay lại.
App vẽ theo toạ độ màn ngang; canvas đổi sang toạ độ panel ngay khi đặt từng điểm.

| Hạng mục | Quy định |
|---|---|
| Kích thước ảnh | 240 × 416 điểm, 30 byte mỗi hàng, 12 480 byte |
| Gói điểm | 8 điểm một byte, điểm đầu ở bit cao nhất |
| Cực tính | Bit 1 là trắng, bit 0 là đen (cùng cực tính với CDI `0x97`/`0xD7` của GxEPD2, xem `docs/hardware/parts/weact-epaper-3.7/image-memory.md`) |
| Toạ độ app | 416 × 240, gốc ở góc trên trái, x sang phải, y xuống dưới |
| Màu | Hai giá trị `BLACK` và `WHITE`; giá trị số của màu bằng đúng bit lưu trong ảnh |
| Cắt mép | Điểm nằm ngoài màn bị bỏ, không báo lỗi |
| Vùng nhớ | Bên gọi đưa vào, ít nhất 12 480 byte; canvas không giữ quyền sở hữu |

Hướng xoay chọn lúc khởi tạo, có hai giá trị. Với toạ độ app (x, y) thì điểm panel (cột, hàng) là:

| Hướng | Cột panel | Hàng panel |
|---|---|---|
| `ROTATION_90` (xoay phải 90°) | y | 415 − x |
| `ROTATION_270` (xoay trái 90°) | 239 − y | x |

Hướng nào đúng với cách gắn màn trên board BB0 chưa xác nhận trên máy, vì điểm đầu tiên của ảnh ứng với góc nào của
panel còn phụ thuộc cách đấu dây.

Phép vẽ có sẵn: xoá cả khung, đặt một điểm, tô hình chữ nhật, vẽ viền hình chữ nhật, vẽ chữ.

### Font

Font là dữ liệu hằng nằm trong flash, do `tools/font_gen.py` sinh từ một file TTF, một cỡ chữ cho mỗi font.

| Hạng mục | Quy định |
|---|---|
| Bitmap mỗi ký tự | 1 bit mỗi điểm, mỗi hàng làm tròn lên số byte, điểm đầu ở bit cao nhất, bit 1 là nét chữ |
| Số đo mỗi ký tự | Mã Unicode (16 bit), rộng, cao, lệch x, lệch y so với đường cơ sở, bước tiến |
| Số đo cả font | Chiều cao dòng, khoảng từ đỉnh dòng tới đường cơ sở; cả hai đủ chứa chữ chồng dấu cao nhất (như Ậ) nên không ký tự nào bị cắt |
| Tra ký tự | Bảng xếp tăng theo mã Unicode, tra nhị phân |
| Bộ ký tự | ASCII in được (U+0020 đến U+007E) và chữ tiếng Việt có dấu dựng sẵn (NFC) |
| Chữ + dấu rời | Không ghép; dấu rời (U+0300 trở đi) không có trong font nên vẽ thành `?` |
| Font đang nhúng | Cascadia Code (giấy phép SIL OFL 1.1), cỡ 24 điểm |

Font phải có ký tự `?`; `font_gen.py` báo lỗi nếu thiếu.

### Vẽ chữ

- Chuỗi vào là UTF-8. Ký tự không có trong font vẽ thành `?`.
- Byte UTF-8 lỗi (byte lẻ, thiếu byte tiếp, mã dài thừa, mã nằm ngoài khoảng Unicode) vẽ thành đúng một `?` cho mỗi đoạn lỗi, rồi giải mã tiếp ngay sau đoạn lỗi, nên một byte lỗi không làm hỏng cả chuỗi.
- Toạ độ truyền vào là góc trên trái của dòng chữ. Hàm vẽ trả về độ rộng đã vẽ; có hàm đo độ rộng không vẽ.
- Canvas không tự xuống dòng; app đo độ rộng rồi tự ngắt dòng.
- Chữ ra ngoài mép bị cắt như mọi phép vẽ khác.
