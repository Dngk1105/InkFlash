# Lưu dữ liệu và đồng bộ

## Vấn đề

Ví dụ một tuần dùng máy:

1. Nạp deck từ máy tính.
2. Ôn 20 thẻ trên máy; đang chấm thẻ thứ 15 thì pin sụt.
3. Ôn tiếp 10 thẻ trên điện thoại, lúc máy không kết nối.
4. Sửa nghĩa một thẻ trên máy tính.
5. Máy đồng bộ: phải có đủ 15 + 10 lần ôn, nghĩa mới nhất, lịch ôn giống hệt điện thoại.

## Cách xử lý

### Tiến độ học là log sự kiện

Mỗi lần chấm điểm thêm một bản ghi vào cuối log, không sửa bản ghi cũ. Mỗi bản ghi có CRC (mã kiểm tra): mất điện
giữa lúc ghi chỉ hỏng bản ghi cuối, lúc đọc phát hiện và bỏ qua.

Trạng thái FSRS của từng thẻ chỉ là bản nháp, tính lại được từ log bất cứ lúc nào (khi đồng bộ, khi đổi tham số FSRS).

### Nhận diện và sửa thẻ

| Việc | Cách làm |
|---|---|
| Mã thẻ | Số ngẫu nhiên 64 bit, do nơi tạo thẻ sinh ra |
| Sửa nội dung | Bản sửa sau thắng; mỗi bản sửa kèm thời điểm và mã thiết bị (phân xử khi trùng giờ) |
| Xoá thẻ | Ghi dấu "đã xoá" (tombstone), không xoá hẳn, để thiết bị khác xoá theo |
| Âm thanh, hình | Tên file là hash của nội dung: trùng nội dung thì lưu một lần, không bao giờ xung đột |

### Nơi lưu và định dạng

| | Cách làm | Lý do |
|---|---|---|
| Dữ liệu chính (deck, log) | LittleFS trên flash trong | Chịu mất điện, tự rải đều số lần ghi |
| microSD (nếu có) | Chỉ media | FAT dễ hỏng khi mất điện; media tải lại được, log thì không |
| Định dạng lưu | Nhị phân tự định nghĩa | Gọn, đọc nhanh |
| Định dạng gửi giữa thiết bị | Tách riêng khỏi định dạng lưu | Đổi cách lưu trong máy không làm hỏng app khác |

### Quy tắc gộp khi đồng bộ

Không phụ thuộc kênh truyền (server, WiFi, Bluetooth, cáp).

| Dữ liệu | Khi hai bên khác nhau |
|---|---|
| Lần chấm điểm | Gộp tất cả, bỏ trùng theo (mã thiết bị, số thứ tự), sắp theo thời gian, tính lại FSRS |
| Nội dung thẻ | Bản sửa sau thắng |
| Xoá | Dấu "đã xoá" thắng nếu mới hơn bản sửa |
| Media | Thiếu file nào thì tải file đó |

Mỗi thiết bị nhớ đã gửi tới số thứ tự nào, mỗi lần chỉ gửi phần mới.

Bản ghi tạo lúc giờ chưa đáng tin (RTC mất giờ sau ship mode hoặc hết pin) được gắn cờ.

### Phiên bản dữ liệu

- Mỗi file có số version ở đầu.
- Firmware mới gặp version cũ thì nâng cấp dữ liệu lên, một lần.
- Firmware cũ gặp version mới hơn thì không ghi đè.

### Khi nào một lần chấm là chắc chắn

Ghi xuống flash ngay sau mỗi lần chấm, không gom trong RAM. Ghi sau khi đã giao khung cho task màn, vì lúc ghi flash
mọi task chạy code từ flash phải chờ (thường vài ms, lúc xoá khối 4 KB khoảng 30–50 ms).

Định dạng bản ghi chính xác do bộ test vector quyết định. Mọi bản code đọc ghi log (máy, điện thoại, Kindle) phải
chạy qua cùng bộ test vector.
