# Viết visualize và mô phỏng

## Vấn đề

Một file mô phỏng viết lại logic theo cách hiểu của người viết, không chép từ code C. Khi code C đổi, file mô phỏng
vẫn chạy và vẫn đẹp, nhưng cho kết quả khác firmware mà không ai biết. Giao diện tự chọn màu, thêm emoji và hiệu ứng
thì khó đọc và lệch với phần còn lại của dự án.

## Cách xử lý

Ví dụ mẫu: `components/core/ink_gesture/test/gesture_sim.html`. File mới làm theo cấu trúc của file này.

### Đặt file ở đâu

| Loại | Chỗ đặt |
|---|---|
| Mô phỏng một khối lõi (C thuần) | `components/core/<khối>/test/<khối>_sim.html` |

Mỗi mô phỏng là một file HTML duy nhất, không thư viện ngoài, không bước build. Mở trực tiếp bằng trình duyệt
(`file://`) là chạy.

### Logic phải khớp code C

Đây là luật quan trọng nhất. Mô phỏng đẹp mà sai thì có hại hơn không có.

- Chép từng hàm của file `.c` sang JS: cùng tên hàm, cùng tên biến, cùng thứ tự lệnh. Không viết lại theo cách hiểu,
  không gộp hay tối ưu.
- Kiểu `uint32_t` của C: bọc phép cộng trừ thời gian bằng `u32(value) = value >>> 0`, phép ép `(int32_t)` bằng
  `i32(value) = value | 0`. Thiếu bước này thì ca đồng hồ quay vòng qua 0 cho kết quả sai.
- Phần chỉ để vẽ (ví dụ ghi lại edge, thời điểm phát gesture) gắn vào qua một hook như `state.trace`, và ghi comment
  rằng phần đó chỉ có ở bản JS.
- Mô phỏng phần vỏ (ví dụ `ink_system`) làm đúng thứ tự trong code vỏ: nhận edge → gọi khối lõi → log → hẹn timer.
  Hằng số lấy đúng giá trị trong code vỏ, kèm comment tên hằng gốc.
- Có tab chạy lại host test: chép từng ca trong `test/test_<khối>.c` sang JS, giữ nguyên các bước (`down`, `up`,
  `advance`, `expect`...) và tên hàm test. Mọi ca phải qua. Ca trượt thì sửa bản JS cho khớp C, không sửa giá trị kỳ vọng.
- Sửa file `.c` hoặc host test thì sửa file mô phỏng trong cùng commit.

### Nội dung nên có

| Phần | Làm gì |
|---|---|
| Điều khiển | Input giống phần cứng (chuột và phím cho nút), và các lệnh console tương ứng (`gesture set`, `gesture enable`) |
| Timeline | Tín hiệu theo thời gian: edge thô, mức sau xử lý, vùng khoá, sự kiện phát ra, deadline kế tiếp |
| State machine | Sơ đồ trạng thái, tô trạng thái hiện tại |
| Trạng thái | Bảng các trường trong `priv`, đúng tên trường C |
| Log | In giống hệt log firmware, để so với log trên board |
| Host test | Danh sách ca test, PASS/FAIL, timeline của từng ca, dấu kiểm tra đặt đúng thời điểm |

Đồng hồ ảo chạy theo giờ thật nhân hệ số tốc độ, có nút tạm dừng và chạy chậm (0.1×). Đồng bộ đồng hồ cả lúc có input,
không chỉ trong `requestAnimationFrame`, vì tab bị ẩn thì `requestAnimationFrame` dừng.

### Giao diện

Theme: Rose Petal DocGraph HC (sáng, độ tương phản cao). Khai báo màu một lần bằng biến CSS trong `:root`, mọi chỗ khác
dùng biến, không viết mã màu rời.

| Biến | Màu | Dùng cho |
|---|---|---|
| `--background` | `#fffafc` | Nền trang |
| `--surface` | `#fbf4f6` | Nền thẻ |
| `--surface-strong` | `#ebdce2` | Thanh tiêu đề, đầu bảng |
| `--highlight` | `#f3dce4` | Mục đang chọn, nút đang bấm |
| `--selection` | `#cca2ae` | Vùng khoá, cạnh sơ đồ |
| `--border` | `#8c3a52` | Viền |
| `--border-soft` | `#d9b2bf` | Viền phụ, tín hiệu thô |
| `--accent` | `#782239` | Nút chính, tab đang mở |
| `--accent-strong` | `#561324` | Tiêu đề |
| `--text` | `#1f120f` | Chữ |
| `--text-muted` | `#63404b` | Chữ phụ |
| `--code-background` | `#f4e6ec` | Nền `code` |
| `--success` | `#096d48` | Kiểm tra qua |
| `--error` | `#be123c` | Kiểm tra trượt, config sai |

Màu theo nghĩa giữ cố định giữa các mô phỏng: nút A `#0f4a85`, nút B `#a16207`, sự kiện phát ra (gesture) `#782239`,
deadline và timer `#5e2cbc`.

Luật:

- Không emoji, không gradient, không bóng mờ, không hiệu ứng chỉ để trang trí. Chuyển động chỉ dùng cho dữ liệu đang chạy.
- Chữ giao diện tiếng Việt; thuật ngữ giữ tiếng Anh theo `docs/writing-comments.md` (edge, debounce, deadline...).
- Font: `"Segoe UI", system-ui` cho chữ, `"Cascadia Code", Consolas, monospace` cho số và tên trong code.
- Layout dùng CSS grid, xuống một cột khi màn hẹp hơn 1100px; không cuộn ngang ở bề rộng 650px.
- Canvas vẽ theo `devicePixelRatio` để không mờ. Mỗi kênh tín hiệu một hàng có nền riêng, ghi rõ mức "bấm"/"nhả".
- Sơ đồ state machine: xếp nút sao cho cạnh không cắt nhau; nhãn cạnh không đè lên nhau hay lên cạnh khác.
- Nhãn sự kiện trên timeline so le hai hàng để không đè nhau.

### Code trong file

- Dòng đầu: `<!-- Thiết kế: docs/design/<file>.md -->`, tiếp theo là comment nói file chép từ file C nào.
- Theo luật đặt tên trong `docs/design/code-structure.md` (viết đủ từ, không viết tắt) và luật comment trong
  `docs/writing-comments.md` (chia khối bằng `/* ---- Tên khối ---- */`).
- Thứ tự khối: hằng số → lõi chép từ C → API lõi → ghi lại để vẽ → vẽ → giả lập vỏ → điều khiển → host test → khởi động.
- Trạng thái của phần chạy trực tiếp gom vào một object (ví dụ `live`), không rải biến toàn cục.

### Kiểm tra trước khi báo xong

1. Mở file bằng trình duyệt: console không có lỗi.
2. Tab host test báo N/N qua, và N bằng số ca trong file C có thể vẽ (bỏ các ca chỉ kiểm tra con trỏ NULL hoặc giá trị enum).
3. Chụp màn ở bề rộng khoảng 650px và 1400px: không chữ đè, không nhãn đè, không cuộn ngang.
4. Thử tay các ca chính trên tab chạy trực tiếp; log khớp log firmware cho cùng thao tác.
