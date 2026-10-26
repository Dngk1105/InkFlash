# Khung app

## Vấn đề

Bản hiện tại có hai app (Flashcard, Cài đặt), nhưng phải chừa đường cho nhiều app.
Máy ngủ sâu thì mất RAM, nên thức dậy phải về đúng màn, đúng thẻ đang xem.

Ví dụ: đang ở màn Đáp án của thẻ #42 thì máy ngủ. Bấm nút, máy phải về lại màn Đáp án của thẻ #42.

## Cách xử lý

### Menu

Máy khởi động vào menu. Menu có ba mục: Flashcard, Cài đặt và Ngủ.

Cài đặt là một app như mọi app khác. Nó đọc và ghi cấu hình thiết bị qua API của `system`, không đụng thẳng NVS.

### Một app có 5 hàm

| Hàm | Khi nào được gọi |
|---|---|
| `start(snapshot)` | Mở app; `snapshot` rỗng nếu mở mới, có thì khôi phục chỗ cũ |
| `intent(i)` | Có thao tác (lật, chấm điểm, BACK...) |
| `draw(canvas)` | Cần vẽ lại |
| `save(snapshot)` | Trước khi ngủ hoặc chuyển app |
| `stop()` | Đóng app, giải phóng bộ nhớ đã cấp ở `start` |

App chạy trong task main và chỉ nhận intent, không biết nút nào. Ngoài 5 hàm, app khai báo danh sách intent của
mình kèm cử chỉ mặc định, và bảng riêng cho scene nào cần nghĩa riêng; Cài đặt đọc danh sách này để người dùng gán
lại cử chỉ. App chỉ gọi `system`, `study`, `display`, `ui`; không gọi thẳng `storage` hay phần cứng.

### Scene trong app

- Mỗi màn của app là một scene, có cùng bộ hàm như app.
- Scene xếp thành ngăn xếp, tối đa 4 tầng; BACK quay về scene trước.
- Bộ quản lý scene viết bằng C thuần, test trên PC.
- Snapshot của app gồm ngăn xếp scene và vài byte trạng thái mỗi scene (ví dụ mã thẻ), lưu trong ô RTC memory
  (vùng RAM còn giữ khi ngủ sâu) của app. Một cơ chế lo cả BACK lẫn thức dậy.

### Việc của system và của app

| Việc | Ai lo |
|---|---|
| Màn nền khi ngủ (số thẻ cần ôn, pin) | `system`, lấy số thẻ từ `study` |
| Giữ A+B ngắn (về menu) / dài (tắt nguồn) | `system` nhận trước, app không thấy |
| BACK ở màn gốc của app | App tự xử lý; app báo không xử lý thì `system` đưa về menu |
| Giữ máy thức (ví dụ đang phát âm thanh) | App xin `system`, có giới hạn thời gian |

### Danh sách app

Cố định, biên dịch sẵn vào firmware. App chỉ gọi qua API của các khối, không dùng biến của khối khác, để sau này
có thể nạp app lúc chạy (như file `.fap` của Flipper Zero).

### Rò rỉ bộ nhớ

Ở bản build debug, `system` đo heap trước và sau mỗi lần chạy app; lệch thì ghi log.
