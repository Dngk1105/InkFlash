# Nút và cử chỉ

## Vấn đề

Máy chỉ có 2 nút (và IMU), nên mỗi nút phải mang nhiều nghĩa: nhấn, giữ, tổ hợp A+B. App không được biết nút nào,
chỉ biết ý định (intent) như "lật thẻ", "chấm Good". Người dùng muốn tự gán cử chỉ nào làm chức năng nào. Quét nút
liên tục thì CPU không được ngủ, tốn pin.

## Cách xử lý

Ví dụ: app Flashcard gán mặc định "B nhấn" → `NEXT`. Người dùng vào Cài đặt đổi `NEXT` sang "B giữ". Từ đó giữ B
thì lật thẻ; code app không đổi.

| Lớp | Đầu vào → đầu ra | Khối |
|---|---|---|
| Cử chỉ | Nút bấm/nhả lúc t, tín hiệu IMU → cử chỉ (nút × kiểu: nhấn, nhấn kép, giữ, A+B...) | `gesture` |
| Bảng gán | Cử chỉ → intent, theo app đang mở | `keymap` |
| App | Intent → hành động | app |

### Khối `gesture` là C thuần

Đầu vào: chuỗi "nút X bấm/nhả lúc t" và tín hiệu IMU. Đầu ra: cử chỉ. Không dính FreeRTOS hay phần cứng, nên mọi
trường hợp khó test được trên PC.

Các ngưỡng thời gian (lọc rung, nhấn kép, giữ, khoảng cách tính là A+B, giữ A+B) nằm trong một struct cấu hình,
đổi được lúc máy đang chạy, không cần build lại.

### Chạy theo sự kiện, không quét

- Ngắt GPIO khi chân nút đổi mức → sự kiện vào hàng đợi task main.
- Hẹn giờ chỉ chạy khi đang có cử chỉ dở (ví dụ đang giữ nút, chờ xem có đủ 2 s không).
- Thời gian được truyền vào làm tham số, `gesture` không tự đọc đồng hồ.

### Ai nhận cử chỉ

| Cử chỉ | Ai xử lý |
|---|---|
| Giữ A+B 2 s | Hệ thống: đi ngủ. App không thấy, không gán lại được |
| Giữ A+B 8 s | Hệ thống: ship mode (ngắt pin). App không thấy, không gán lại được |
| Mọi cử chỉ khác | Tra bảng gán ra intent, đưa cho app đang mở. Cử chỉ chưa gán thì bỏ |

### Bảng gán (`keymap`)

- Mỗi app khai báo danh sách intent của mình: mã, tên hiện trong Cài đặt, cử chỉ mặc định.
- Intent chung (`NEXT`, `PREV`, `SELECT`, `BACK`) có mã cố định, dùng chung mọi app. Intent riêng của app đánh mã
  sau các mã chung.
- Bảng gán theo app. Scene nào cần nghĩa riêng thì app khai báo thêm bảng cho scene đó: tra bảng scene trước, không
  có thì tra bảng app.
- Khi lưu, `keymap` kiểm tra: mỗi cử chỉ chỉ trỏ tới một intent, và `BACK` luôn được gán, để người dùng không bị kẹt
  trong app.
- Cài đặt có mục khôi phục bảng gán mặc định.
- Phần tra và kiểm tra là C thuần, test trên PC. `system` giữ bảng của app đang mở và lưu bảng người dùng đã sửa.

### Lần bấm đánh thức

Lần bấm làm máy thức không tính là thao tác: lần nhả đầu tiên mà không thấy lần nhấn tương ứng thì bỏ qua.

### Nhấn kép làm chậm nhấn đơn

Nút nào có nhấn kép thì mỗi lần nhấn đơn phải chờ hết khoảng chờ nhấn kép (thường ~400 ms) mới biết là nhấn đơn.
Chỉ bật nhấn kép cho nút thật sự cần: khi mở app, `system` bật nhận nhấn kép cho nút nào có cử chỉ nhấn kép trong
bảng gán của app đó.

### IMU

IMU chỉ dùng khi máy thức (gõ, úp mặt, xoay màn). Không dùng IMU để đánh thức máy.

Driver IMU khai báo nó có tự nhận cú gõ, độ nghiêng không. Có thì dùng ngắt của chip IMU, đi đường giống nút.
Không có thì `gesture` tự tính từ dữ liệu IMU.

## Tham khảo

OneButton (thư viện Arduino): mỗi nút một máy trạng thái INIT → DOWN → UP → COUNT → PRESS. Học cách tách đọc chân
khỏi logic (`tick(bool level)`). Không chép cách gọi `tick()` liên tục và tự đọc `millis()`.
