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
| Điều khiển | Input giống phần cứng (chuột và phím cho nút), và các cài đặt tương ứng (ngưỡng gesture, mask kiểu phải chờ) |
| Timeline | Tín hiệu theo thời gian: edge thô, mức sau xử lý, vùng khoá, sự kiện phát ra, deadline kế tiếp |
| State machine | Sơ đồ trạng thái, tô trạng thái hiện tại |
| Trạng thái | Bảng các trường trong `priv`, đúng tên trường C |
| Log | In giống hệt log firmware, để so với log trên board |
| Host test | Danh sách ca test, PASS/FAIL, timeline của từng ca, dấu kiểm tra đặt đúng thời điểm |

Đồng hồ ảo chạy theo giờ thật nhân hệ số tốc độ, có nút tạm dừng và chạy chậm (0.1×). Đồng bộ đồng hồ cả lúc có input,
không chỉ trong `requestAnimationFrame`, vì tab bị ẩn thì `requestAnimationFrame` dừng.

### Giao diện

Mọi trang chạy trên trình duyệt trong repo (mô phỏng, sơ đồ, công cụ test ở nhánh `bench`) dùng preset Editorial của
Archify, có cả sáng và tối. Ví dụ đang dùng: `components/core/ink_gesture/test/gesture_sim.html` và
`components/core/ink_display_policy/test/display_policy_sim.html`.

Thẻ `<html>` mang `data-preset="editorial"` và `data-theme="light"` hoặc `"dark"`. Màu khai báo một lần bằng biến CSS
cho từng theme, mọi chỗ khác dùng biến, không viết mã màu rời. Canvas không tự đổi theo CSS: đọc biến bằng
`getComputedStyle(document.documentElement)` và vẽ lại khi đổi theme.

| Biến | Sáng | Tối | Dùng cho |
|---|---|---|---|
| `--bg` | `#f2eee5` | `#181611` | Nền trang |
| `--grid` | `#d8d0c2` | `#39342a` | Dòng kẻ giấy của nền |
| `--panel` | `#fbf8f1` | `#231f18` | Nền thẻ, nút |
| `--panel-strong` | `#ede6d8` | `#2e2921` | Thanh tab, đầu bảng, nền log |
| `--panel-border` | `#c4b9a6` | `#625a4a` | Viền |
| `--panel-border-soft` | `#dcd4c5` | `#443e33` | Viền phụ, đường kẻ trong thẻ |
| `--text` | `#242018` | `#f4eddf` | Chữ |
| `--text-muted` | `#6f6658` | `#b9ae9b` | Chữ phụ, nhãn |
| `--text-dim` | `#948a79` | `#776e60` | Chữ mờ, mục trống |
| `--accent` | `#9f403d` | `#cf6662` | Nút chính, tab đang mở, tiêu đề thẻ |
| `--accent-strong` | `#7d2a27` | `#f4eddf` | Tiêu đề trang |
| `--accent-soft` | `#f4dcdb` | `#382424` | Mục đang chọn |
| `--brand-teal` | `#287e84` | `#7fc6c7` | Nút A, window |
| `--brand-bronze` | `#9a671f` | `#d8ad68` | Nút B, cảnh báo |
| `--brand-green` | `#397b53` | `#8fc29e` | Kiểm tra qua, đã kết nối |
| `--brand-purple` | `#765d86` | `#c0a4d0` | Deadline, timer, log của trang |
| `--error` | `#b42318` | `#f0857d` | Kiểm tra trượt, lỗi |
| `--pixel-white`, `--pixel-black` | `#ffffff`, `#1f1d1a` | `#e9e3d5`, `#0c0b09` | Điểm trắng, đen của màn e-ink |

Màu theo nghĩa giữ cố định giữa các trang: nút A `--brand-teal`, nút B `--brand-bronze`, sự kiện phát ra (gesture)
`--accent`, deadline và timer `--brand-purple`.

Luật:

- Không emoji, không bóng đổ, không hiệu ứng chỉ để trang trí. Gradient chỉ dùng cho nền giấy kẻ dòng của preset.
  Chuyển động chỉ dùng cho dữ liệu đang chạy.
- Theme mặc định theo hệ điều hành (`prefers-color-scheme`); có nút đổi sáng/tối ghi chữ ("Sáng", "Tối"), nhớ lựa chọn
  bằng `localStorage` bọc trong `try`.
- Chữ giao diện tiếng Việt; thuật ngữ giữ tiếng Anh theo `docs/writing-comments.md` (edge, debounce, deadline...).
- Font: `--font-serif` (`"Newsreader", "Charter", "Cambria", "Georgia", serif`) cho tiêu đề trang và tiêu đề thẻ viết
  hoa; `--font-sans` (`"Segoe UI", system-ui`) cho chữ; `--font-mono` (`"Cascadia Code", Consolas, monospace`) cho số
  và tên trong code. Cambria đứng trước Georgia vì Georgia thiếu chữ Việt hoa có hai dấu.
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
3. Chụp màn ở bề rộng khoảng 650px và 1400px, cả sáng lẫn tối: không chữ đè, không nhãn đè, không cuộn ngang.
4. Thử tay các ca chính trên tab chạy trực tiếp; log khớp log firmware cho cùng thao tác.
