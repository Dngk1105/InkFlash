# UI: canvas và họa cụ

## Vấn đề

App cần vẽ chữ, hình và bố cục lên frame mà không viết lại cùng một đoạn code ở mỗi màn. Ba việc lặp lại nhiều nhất:
ghi từng bit vào frame, xuống dòng chữ khi cỡ chữ hoặc bề rộng đổi, và tính toạ độ bằng tay.

Ví dụ: app Flashcard lật thẻ. App vẽ viền thẻ rồi submit với intent `UI`. Sau đó app vẽ chữ vào chính frame đó rồi
submit với intent `TEXT`. Người dùng thấy viền hiện trước, chữ hiện sau. Thứ tự này do app chọn, vì chỉ app biết nội
dung nào cần nhanh và nội dung nào cần rõ.

Panel cũng có thể đổi: cỡ khác, chiều khác, sau này có thể nhiều mức xám hoặc có màu. Code của app không được viết
lại khi panel đổi.

## Cách xử lý

### App là người vẽ, `ui` là hộp họa cụ

| Việc | Ai làm |
|---|---|
| Acquire frame, quyết định vẽ gì và vẽ ở đâu, chia thành mấy frame, chọn intent, submit | App |
| Ghi pixel, vẽ chữ, đo chữ, chia chữ nhật | `ui` |
| Tìm changed window, chọn waveform, cleanup | `display` |

- `ui` không gọi `ink_display_acquire()` và không gọi `ink_display_submit()`. App bind canvas vào frame vừa acquire,
  vẽ, rồi tự submit.
- `ui` không giữ danh sách những gì đã vẽ. Canvas chỉ có pixel của frame. App muốn đổi một vùng thì tô lại vùng đó
  rồi vẽ đè lên.
- `ui` không biết app, danh sách app hay cấu hình của người dùng. App đọc những thứ đó rồi truyền vào bằng tham số.
- `ui` là core module: không include ESP-IDF, có host test trên PC.

### Canvas

Canvas là một frame kèm ba thứ: frame info (rộng, cao, pixel format), góc xoay và vùng clip.

| Việc | Cách làm |
|---|---|
| Khởi tạo | Một lần, từ frame info do `display` trả về và góc xoay do app chọn |
| Bind | Mỗi lần acquire: gắn canvas vào frame vừa acquire. Canvas không cấp phát bộ nhớ ảnh |
| Toạ độ | App vẽ theo toạ độ sau khi xoay. Canvas đổi sang chiều gốc của panel lúc ghi pixel, nên frame luôn đúng layout mà `display` nhận |
| Clip | Lệnh vẽ ngoài vùng clip không ghi pixel nào |
| Truy cập thẳng | App lấy được con trỏ frame và frame info từ canvas để tự ghi pixel khi họa cụ không đủ |

Pixel format hiện có một giá trị: `K1`, 1 bit mỗi pixel, bit 1 là trắng. Canvas từ chối frame info có pixel format
khác.

### Họa cụ

| Nhóm | Có gì |
|---|---|
| Primitive | Tô chữ nhật, viền chữ nhật theo độ dày, bo góc, đường thẳng, bitmap 1 bit |
| Paint | Màu mong muốn (đỏ, lục, lam, mỗi kênh 8 bit) và cách ghi: ghi đè hoặc đảo pixel (XOR) |
| Chữ | Font, đo kích thước một đoạn chữ khi chưa vẽ, vẽ chữ UTF-8 trong một chữ nhật: xuống dòng theo từ, giới hạn số dòng, cắt bằng dấu ba chấm, căn trái, giữa, phải |
| Thước | Thụt lề một chữ nhật, chia chữ nhật thành hàng hoặc cột theo phần cố định và phần co giãn, căn một khối trong chữ nhật |

Paint mang màu mong muốn, không mang bit. Canvas đổi màu đó thành pixel theo pixel format của frame. Với `K1`:

| Độ sáng của màu | Tô chữ nhật | Viền, đường thẳng, chữ, bitmap |
|---|---|---|
| Đen hoặc trắng | Pixel đen hoặc trắng | Pixel đen hoặc trắng |
| Mức giữa | Pattern dither 4 × 4 theo độ sáng | Đen nếu độ sáng dưới 50 %, còn lại trắng |

Nhờ vậy app viết "xám 50 %" một lần. Panel 1 bit hiện pattern chấm; panel có pixel format khác sẽ hiện theo cách
của pixel format đó mà app không đổi code.

### Font

Font là dữ liệu trong flash, theo định dạng font của U8g2. `tools/u8g2_font.py` chép một font theo tên từ mã nguồn
U8g2 ra một file `.c` trong `components/core/ink_ui/fonts/`. Hiện có một font: GNU Unifont 16 px, có đủ dấu tiếng
Việt.

### U8g2 nằm sau API của `ui`

`ui` dùng thư viện U8g2 để vẽ primitive và chữ lên frame `K1`. App chỉ gọi hàm `ink_ui_*` và không thấy kiểu nào của
U8g2. Đổi thư viện vẽ, hoặc thêm backend vẽ cho pixel format khác, không đụng tới app.

| Khối | Việc |
|---|---|
| `third_party/u8g2` | Mã nguồn U8g2, là git submodule, ghim theo một commit |
| `ink_u8g2` | Component bọc: chọn các file U8g2 cần biên dịch và dựng một màn ảo trỏ vào frame |
| `ink_ui` | API cho app; gọi `ink_u8g2` |

U8g2 ghi buffer theo từng hàng, pixel trái nằm ở bit cao nhất, trùng layout frame `K1`. Vì vậy U8g2 ghi thẳng vào
frame, không có bước chép đổi dạng.

### Vẽ thử trên board

`ink_demo` là shell module tạm, tồn tại tới khi có app đầu tiên. `system` gọi nó trên main task: sau khi boot nó vẽ
màn thử thứ nhất; gesture `PRESS` của nút A sang màn kế, của nút B về màn trước. Bốn màn thử: primitive và paint,
chữ tiếng Việt, thước và XOR, vẽ hai frame liên tiếp với intent `UI` rồi `TEXT`.

## Tham khảo

- U8g2: thư viện đồ hoạ đơn sắc viết bằng C, giấy phép BSD 2 điều khoản; font đi kèm có giấy phép riêng theo từng font.
- Flipper Zero: API canvas cho app, app tự vẽ trong hàm vẽ của mình.
