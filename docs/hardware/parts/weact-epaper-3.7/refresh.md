# Module e-paper WeAct 3.7": cơ chế làm mới và xử lý hình ảnh

Panel E037A75-A0 (240×416), controller tích hợp UC8253.

## Các chế độ làm mới

| Chế độ làm mới | Thời gian đo được | Màn chớp | Mục đích sử dụng | Ràng buộc kỹ thuật |
|---|---|---|---|---|
| **Full refresh (Chuẩn)** | 2,8 s | Có (1-2 lần) | Vẽ trang mới, chuyển bài học, lật thẻ | Nạp đủ 12.480 byte, dùng bảng sóng OTP chuẩn |
| **Sóng nhanh (Fast full)** | 2,3 s | Có | Rút ngắn thời gian chờ khi lật trang | Ép nhiệt độ bằng lệnh phụ `0xE0 0x02` + `0xE5 0x5A` |
| **Partial refresh (Từng phần)** | ~0,35 s | Không chớp | Cập nhật đồng hồ, thanh tiến độ, icon pin | Tọa độ X bắt buộc phải căn theo byte (bội số của 8 bit) |
| **Deep clean (Làm sạch sâu)** | 8,3 s | Có (Đen → Trắng) | Xóa triệt để bóng mờ và chấm lốm đốm | Quét 1 chu kỳ toàn đen rồi toàn trắng trước khi vẽ ảnh |

## Xử lý bóng mờ (Ghosting) và chấm lốm đốm

1. **Nguyên nhân tích bóng mờ**:
   - Khi chạy Partial refresh liên tiếp, các hạt micro-capsule chịu xung điện một chiều cục bộ mà không được đảo cực cân bằng, dẫn đến hạt mực bị dính lại lớp kính hoặc tích tụ điện tích dư thừa.
   - Hiện tượng biểu hiện bằng vết mờ của chữ/hình cũ dưới nền trắng, hoặc xuất hiện chấm li ti ở mép màn hình.
2. **Quy tắc vận hành trên máy bỏ túi**:
   - Chỉ dùng Partial refresh cho các thay đổi nhỏ, cục bộ (thời gian học, đổi thẻ nhanh).
   - Sau mỗi 20 đến 30 lần Partial refresh: bắt buộc thực hiện một lần Full refresh hoặc Deep clean.
   - Khi chuyển đổi giữa hai màn hình giao diện khác hẳn nhau (từ menu sang bài học): luôn dùng Full refresh.

## Ràng buộc nhiệt độ môi trường

| Dải nhiệt độ | Trạng thái hiển thị | Khuyến nghị vận hành |
|---|---|---|
| **15 °C ~ 35 °C** | Tối ưu | Hạt mực di chuyển nhanh nhất, độ tương phản 10:1 đạt chuẩn. |
| **0 °C ~ 15 °C** | Chậm hơn | Độ nhớt dung dịch điện di tăng, thời gian làm mới kéo dài hơn bình thường. |
| **> 35 °C** | Dễ suy giảm | Độ tương phản giảm nhẹ, dễ xuất hiện chấm đen. Khuyến nghị quét refresh định kỳ mỗi 24h nếu giữ màn tĩnh. |
| **< 0 °C hoặc > 40 °C** | Cấm hoạt động | Không kích hoạt lệnh refresh vì có thể gây tổn hại vĩnh viễn cấu trúc hạt mực. |
