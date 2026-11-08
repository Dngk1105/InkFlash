# App framework

## Vấn đề

Bản hiện tại có hai app (Flashcard, Settings), nhưng phải chừa đường cho nhiều app. Deep sleep làm mất RAM, nên wake
xong máy phải về đúng màn, đúng thẻ đang xem.

Ví dụ: đang ở màn Đáp án của thẻ #42 thì máy ngủ. Bấm nút, máy phải về lại màn Đáp án của thẻ #42.

## Cách xử lý

### Menu

Máy boot vào menu. Menu có ba mục: Flashcard, Settings và Ngủ.

Settings là một app như mọi app khác. Settings đọc và ghi cấu hình thiết bị qua API của `system`, không đụng thẳng NVS.

### Một app có 5 hàm

| Hàm | Khi nào được gọi |
|---|---|
| `start(snapshot)` | Mở app; `snapshot` rỗng nếu mở mới, có thì khôi phục chỗ cũ |
| `command(c)` | Có command (lật, chấm điểm, `BACK`...) |
| `draw(canvas)` | Cần vẽ lại |
| `save(snapshot)` | Trước khi ngủ hoặc chuyển app |
| `stop()` | Đóng app, giải phóng bộ nhớ đã cấp ở `start` |

App chạy trên main task và chỉ nhận command, không biết nút nào. Ngoài 5 hàm, app khai báo danh sách command
của mình kèm gesture mặc định (app table), và scene table cho scene nào cần nghĩa riêng; Settings đọc danh sách này để
người dùng gán lại gesture. App chỉ gọi `system`, `study`, `display`, `ui`; không gọi thẳng `storage` hay phần cứng.

### Scene trong app

- Mỗi màn của app là một scene, có cùng bộ hàm như app.
- Scene xếp thành scene stack, tối đa 4 tầng; `BACK` quay về scene trước.
- Module quản lý scene là core module, test trên PC.
- Snapshot của app gồm scene stack và vài byte state mỗi scene (ví dụ mã thẻ), lưu trong RTC slot của app (RTC memory
  là vùng RAM còn giữ khi deep sleep). Một cơ chế lo cả `BACK` lẫn wake.

### Việc của system và của app

| Việc | Ai lo |
|---|---|
| Background frame khi ngủ (số thẻ cần ôn, pin) | `system`, lấy số thẻ từ `study` |
| `HOLD_AB_SHORT` (về menu), `HOLD_AB_LONG` (tắt nguồn) | `system` nhận trước, app không thấy |
| `BACK` ở scene gốc của app | App tự xử lý; app báo không xử lý thì `system` đưa về menu |
| Giữ máy thức (ví dụ đang phát âm thanh) | App xin `system`, có giới hạn thời gian |

### Danh sách app

Danh sách app cố định, biên dịch sẵn vào firmware. App chỉ gọi qua API của các module, không dùng biến của module khác,
để sau này nạp được app lúc chạy (như file `.fap` của Flipper Zero).

### Rò rỉ bộ nhớ

Ở bản build debug, `system` đo heap trước và sau mỗi lần chạy app; lệch thì ghi log.
