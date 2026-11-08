# Canvas và chữ

## Vấn đề

App nghĩ theo màn ngang 416 × 240 px, nhưng panel lưu ảnh dọc 240 × 416 px, và `display` đưa nguyên frame cho driver.
App vẽ theo màn ngang rồi xoay cả frame sang bố cục panel thì tốn thêm 12 480 byte RAM và thời gian ở đoạn 2 của đường
vẽ (đoạn này phải dưới 50 ms). Chữ phải có tiếng Việt có dấu, mà máy không có font hệ thống.

## Cách xử lý

`ui` là core module. `ui` không tự cấp phát bộ nhớ: caller đưa vùng nhớ frame vào lúc init canvas.

### Canvas

Canvas lưu ảnh đúng bố cục RAM ảnh của panel, nên `display` đưa thẳng frame cho driver, không chép hay xoay lại. App
vẽ theo app coordinates; canvas đổi sang toạ độ panel ngay khi đặt từng pixel.

| Hạng mục | Quy định |
|---|---|
| Kích thước ảnh | 240 × 416 px, 30 byte mỗi hàng, 12 480 byte |
| Gói pixel | 8 pixel một byte, pixel đầu ở bit cao nhất |
| Cực tính | Bit 1 là trắng, bit 0 là đen (cùng cực tính với CDI `0x97`/`0xD7` của GxEPD2, xem `docs/hardware/parts/weact-epaper-3.7/image-memory.md`) |
| App coordinates | 416 × 240 px, gốc ở góc trên trái, x sang phải, y xuống dưới |
| Màu | Hai giá trị `BLACK` và `WHITE`; giá trị số của màu bằng đúng bit lưu trong ảnh |
| Cắt mép | Pixel nằm ngoài màn không được vẽ, không báo lỗi |
| Vùng nhớ | Caller đưa vào, ít nhất 12 480 byte; canvas không giữ quyền sở hữu |

Rotation chọn lúc init canvas, có hai giá trị. Với app coordinates (x, y), pixel panel (cột, hàng) là:

| Rotation | Cột panel | Hàng panel |
|---|---|---|
| `ROTATION_90` (xoay phải 90°) | y | 415 − x |
| `ROTATION_270` (xoay trái 90°) | 239 − y | x |

Rotation nào đúng với cách gắn màn trên board BB0 chưa xác nhận trên máy, vì pixel đầu tiên của ảnh ứng với góc nào
của panel còn phụ thuộc cách đấu dây.

Phép vẽ có sẵn: xoá cả canvas, đặt một pixel, tô hình chữ nhật, vẽ viền hình chữ nhật, vẽ chữ.

### Font

Font là dữ liệu hằng nằm trong flash. `tools/font_gen.py` sinh font từ một file TTF, một cỡ chữ cho mỗi font.

| Hạng mục | Quy định |
|---|---|
| Bitmap mỗi glyph | 1 bit mỗi pixel, mỗi hàng làm tròn lên số byte, pixel đầu ở bit cao nhất, bit 1 là nét chữ |
| Số đo mỗi glyph | Mã Unicode (16 bit), rộng, cao, lệch x, lệch y so với baseline, advance |
| Số đo cả font | `line_height`, và `ascent` (từ đỉnh dòng tới baseline); cả hai đủ chứa chữ chồng dấu cao nhất (như Ậ), nên không glyph nào bị cắt |
| Tra glyph | Bảng xếp tăng theo mã Unicode, tra nhị phân |
| Bộ ký tự | ASCII in được (U+0020 đến U+007E) và chữ tiếng Việt có dấu dựng sẵn (NFC) |
| Chữ + dấu rời | Không ghép; dấu rời (U+0300 trở đi) không có trong font nên vẽ thành `?` |
| Font đang nhúng | Cascadia Code (giấy phép SIL OFL 1.1), cỡ 24 |

Font phải có glyph `?`; `font_gen.py` báo lỗi nếu thiếu.

### Vẽ chữ

- Chuỗi vào là UTF-8. Ký tự không có trong font vẽ thành `?`.
- Byte UTF-8 lỗi (byte lẻ, thiếu byte tiếp, mã dài thừa, mã nằm ngoài khoảng Unicode) vẽ thành đúng một `?` cho mỗi
  đoạn lỗi, rồi giải mã tiếp ngay sau đoạn lỗi. Một byte lỗi không làm hỏng cả chuỗi.
- Toạ độ truyền vào là góc trên trái của dòng chữ. `ink_ui_draw_text()` trả về độ rộng đã vẽ;
  `ink_ui_measure_text()` đo độ rộng mà không vẽ.
- Canvas không tự xuống dòng; app đo độ rộng rồi tự chia dòng.
- Chữ ra ngoài mép bị cắt như mọi phép vẽ khác.
