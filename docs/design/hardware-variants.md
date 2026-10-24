# Một firmware, nhiều phần cứng

## Vấn đề

Trong lúc phát triển có ba thứ đổi độc lập với nhau:

- Code firmware: đổi hằng ngày, nhiều bản build khác nhau.
- Phần cứng: breadboard, các bản PCB, linh kiện thay thế khi hỏng hoặc hết hàng.
- Dữ liệu trên flash: định dạng có thể đổi giữa các bản firmware.

Nếu không quản lý, sẽ gặp: nạp nhầm firmware cho board khác, code của board ít dùng bị hỏng mà không ai biết,
không biết một máy đang chạy bản nào.

## Cách xử lý

### 1. Mọi thứ đều có mã

| Thứ | Mã | Ở đâu |
|---|---|---|
| Bản firmware | Lấy từ `git describe`, ví dụ `v0.3-12-g8dbe279` | ESP-IDF tự gắn lúc build |
| Board | `BB0` (breadboard), `PCB1`, `PCB2`... | In trên mặt PCB |

Lúc khởi động, firmware in một dòng: bản firmware, board, các chip dò được.

### 2. Ba mức tách phần cứng

| | Mức 1 | Mức 2 | Mức 3 |
|---|---|---|---|
| Chọn board lúc nào | Lúc build | Lúc build | Lúc chạy |
| Số file firmware | Mỗi board một file | Mỗi board một file | Một file cho mọi board |
| Code phân biệt board (`#if BOARD`) nằm ở | Khắp nơi | Chỉ trong `bsp/` | Không có |
| Tầng trên biết phần cứng làm được gì nhờ | `#if` | Bảng khả năng do driver khai báo | Bảng khả năng do driver khai báo |
| Code của board ít dùng hỏng âm thầm | Dễ | Ít | Không |

Bảng khả năng: driver tự khai báo nó làm được gì, ví dụ IMU có tự nhận cú gõ không, màn có partial refresh không,
cỡ màn bao nhiêu. Tầng trên đọc bảng này để tự điều chỉnh, không hỏi tên chip.

### 3. Đang áp dụng: mức 2, chuẩn bị sẵn cho mức 3

- `#if BOARD` chỉ được viết trong `bsp/`.
- Bảng chân GPIO là dữ liệu (một bảng cho mỗi board), không dùng `#define`.
- Driver nhận chân qua tham số lúc khởi tạo, không tự biết chân.
- Trước khi commit, build cho mọi board.

### 4. Khi nào lên mức 3

Khi có từ hai board cùng được dùng. Lúc đó:

- Firmware biết board nhờ điện trở chia áp nối vào một chân ADC trên PCB. Không có điện trở thì là breadboard.
- IMU được dò bằng chip ID trên I2C.
- Chỉ dò ở lần khởi động nguội (cấp điện lần đầu), kết quả lưu vào RTC memory để các lần thức sau không phải dò lại.

Vì vậy mọi bản PCB phải có điện trở chia áp ngay từ bản đầu tiên.

## Tham khảo

- Zephyr devicetree: mô tả phần cứng bằng dữ liệu.
- QMK: mỗi board bàn phím chỉ là vài file cấu hình.
- Flipper Zero `furi_hal_version`: đọc phiên bản phần cứng ghi trong OTP lúc sản xuất.
