# Nút và cử chỉ

## Vấn đề

Máy chỉ có 2 nút (và IMU), nên mỗi nút phải mang nhiều nghĩa: nhấn, giữ, tổ hợp A+B. App không được biết nút nào,
chỉ biết ý định (intent) như "lật thẻ", "chấm Good". Quét nút liên tục thì CPU không được ngủ, tốn pin.

## Cách xử lý

### Khối `gesture` là C thuần

Đầu vào: chuỗi "nút X bấm/nhả lúc t" và tín hiệu IMU. Đầu ra: intent. Không dính FreeRTOS hay phần cứng, nên mọi
trường hợp khó test được trên PC.

### Chạy theo sự kiện, không quét

- Ngắt GPIO khi chân nút đổi mức → sự kiện vào hàng đợi task main.
- Hẹn giờ chỉ chạy khi đang có cử chỉ dở (ví dụ đang giữ nút, chờ xem có đủ 2 s không).
- Thời gian được truyền vào làm tham số, `gesture` không tự đọc đồng hồ.

### Ai nhận intent

| Cử chỉ | Ai xử lý |
|---|---|
| Giữ A+B 2 s | Hệ thống: đi ngủ. App không thấy |
| Giữ A+B 8 s | Hệ thống: ship mode (ngắt pin). App không thấy |
| Mọi cử chỉ khác | App đang mở |

### Lần bấm đánh thức

Lần bấm làm máy thức không tính là thao tác: lần nhả đầu tiên mà không thấy lần nhấn tương ứng thì bỏ qua.

### Nhấn kép làm chậm nhấn đơn

Nút nào có nhấn kép thì mỗi lần nhấn đơn phải chờ hết khoảng chờ nhấn kép (thường ~400 ms) mới biết là nhấn đơn.
Chỉ bật nhấn kép cho nút thật sự cần.

### IMU

IMU chỉ dùng khi máy thức (gõ, úp mặt, xoay màn). Không dùng IMU để đánh thức máy.

Driver IMU khai báo nó có tự nhận cú gõ, độ nghiêng không. Có thì dùng ngắt của chip IMU, đi đường giống nút.
Không có thì `gesture` tự tính từ dữ liệu IMU.

## Tham khảo

OneButton (thư viện Arduino): mỗi nút một máy trạng thái INIT → DOWN → UP → COUNT → PRESS. Học cách tách đọc chân
khỏi logic (`tick(bool level)`). Không chép cách gọi `tick()` liên tục và tự đọc `millis()`.
