# Module e-paper WeAct 3.7": chỉ mục tra cứu

Thư mục này là cơ sở tri thức về màn e-paper 3,7". Mỗi file trả lời một nhóm câu hỏi. Đọc bảng dưới đây để biết mở file nào, đừng đọc hết.

## Ví dụ tra cứu

Câu hỏi: "Gửi ảnh xong thì phải làm gì để màn đổi hình?" Tra bảng: dòng "Thứ tự gửi lệnh" dẫn tới `sequences.md`. Trong đó có bước gửi lệnh `0x12` (Display Refresh) rồi chờ chân BUSY lên cao. Muốn biết lệnh `0x12` có tham số gì thì sang `commands.md`. Muốn xem chính câu chữ của hãng thì mở trang 20 của `source/uc8253-datasheet.pdf`.

## Tìm gì ở đâu

| Câu hỏi | Mở file | Tài liệu gốc để đối chiếu (trong `source/`) |
|---|---|---|
| Module này là gì, panel và controller nào, thông số chính | `overview.md` | `panel-e037a75-a0-spec.pdf` tr.4–5 |
| Chân nào làm gì, nối với ESP32 thế nào, FPC 24 chân | `pins.md` | `module-schematic-4.2-3.7.pdf`, `module-board-shape-3.7.pdf` |
| Cấp nguồn bao nhiêu V, dòng tiêu thụ, nhiệt độ, độ ẩm, giới hạn | `electrical.md` | `panel-e037a75-a0-spec.pdf` tr.11–12; `uc8253-datasheet.pdf` tr.58–59 |
| Kích thước, lỗ bắt vít, đầu nối, lưu ý lắp | `mechanical.md` | `module-board-shape-3.7.pdf`; `panel-e037a75-a0-spec.pdf` tr.5–6, 14 |
| SPI, mức logic, reset, chân BUSY, deep sleep | `interface.md` | `uc8253-datasheet.pdf` tr.6–7, 41–46, 53, 60–61 |
| Dữ liệu ảnh nằm trong RAM thế nào, bit 1 là đen hay trắng, partial window, vì sao lần vẽ thứ hai không đổi hình (vai OLD/NEW đổi sau mỗi lần làm mới) | `image-memory.md` | `uc8253-datasheet.pdf` tr.20–21, 29, 32, 37; `gdey037t03-spec.pdf` tr.39 |
| Lệnh nào, tham số gì, mặc định bao nhiêu | `commands.md` | `uc8253-datasheet.pdf` tr.8–40 |
| Dạng sóng, bảng LUT, vì sao nhiệt độ ảnh hưởng, ý nghĩa `E0`/`E5`, nội dung OTP của module, nạp LUT qua thanh ghi | `waveform.md` | `uc8253-datasheet.pdf` tr.22–26, 47–52 |
| Thứ tự gửi lệnh: khởi tạo, vẽ, ngủ, đánh thức | `sequences.md` | `panel-vendor-init-uc8253.c`; `gdey037t03-spec.pdf` tr.39; `uc8253-datasheet.pdf` tr.43–46 |
| Thời gian làm mới, bảng sóng ngắn, bóng mờ, cách chống | `refresh.md` | Không có số liệu trong tài liệu hãng; xem file |
| Từ viết tắt và thuật ngữ | `glossary.md` | |
| Danh mục tài liệu gốc, trang nào chứa gì | `sources.md` | |

## Khi không tìm ra thông tin

Làm theo thứ tự này, dừng ở bước đầu tiên có câu trả lời:

1. Tìm trong file docs ở bảng trên. Mỗi file có dòng "Đối chiếu gốc" ở cuối, chỉ thẳng tới trang PDF.
2. Mở PDF gốc trong `source/` theo trang ghi ở `sources.md`. Datasheet UC8253 có text tìm được; bảng thanh ghi trong đó bị bể cột khi trích text, nên nhìn trực tiếp trang PDF cho chắc.
3. Sơ đồ nguyên lý và bản vẽ hình dạng (`module-schematic-4.2-3.7.pdf`, `module-board-shape-3.7.pdf`) là hình vector: mở bằng trình xem PDF, đừng trông chờ vào text.
4. Thư mục gốc đầy đủ của WeAct (có thêm file STEP 3D và ví dụ các hãng khác) nằm trên máy dev, đường dẫn ghi ở `sources.md`.
5. Vẫn không có: hỏi người làm. Không đoán thông số phần cứng.

## Quy ước trạng thái

Các bảng thông số có cột "Trạng thái":

| Giá trị | Nghĩa |
|---|---|
| `Đã xác nhận` | Đã đo trên máy, hoặc đã đối chiếu với tài liệu hãng trong `source/` (nguồn ghi ở cột bên cạnh) |
| `Chưa xác nhận` | Chỉ có từ thư viện GxEPD2, ví dụ code, hoặc suy luận từ tài liệu; chưa đo, chưa có câu chữ hãng khẳng định |
