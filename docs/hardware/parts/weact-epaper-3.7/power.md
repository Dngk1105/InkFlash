# Module e-paper WeAct 3.7": nguồn và tiêu thụ điện

Panel E037A75-A0, driver tích hợp UC8253, ổn áp LDO ME6231A33M3G.

## Thông số điện áp

| Đại lượng | Giá trị giới hạn | Giá trị hoạt động khuyến nghị | Đơn vị | Ghi chú |
|---|---|---|---|---|
| Điện áp cấp logic ($V_{DDIO}$) | -0,5 ~ +6,0 | 2,4 ~ 3,6 (chuẩn 3,3) | V | Tương thích mức logic 3,3 V của ESP32-S3 |
| Điện áp nguồn lõi ($V_{CC} / V_{DD}$) | -0,5 ~ +6,0 | 2,4 ~ 3,6 | V | Cấp cho khối analog và controller |
| Mức điện áp vào cao ($V_{IH}$) | - | 0,8 × $V_{DDIO}$ ~ $V_{DDIO}$ | V | Tối thiểu 2,64 V tại nguồn 3,3 V |
| Mức điện áp vào thấp ($V_{IL}$) | - | 0 ~ 0,2 × $V_{DDIO}$ | V | Tối đa 0,66 V tại nguồn 3,3 V |

## Tiêu thụ dòng theo trạng thái

| Trạng thái | Dòng tiêu thụ | Thời gian duy trì | Nguồn |
|---|---|---|---|
| Deep Sleep (Ngủ sâu) | **~3 µA** | Suốt thời gian màn giữ ảnh tĩnh | Datasheet UC8253 |
| Chờ nhận lệnh (Idle/Standby) | ~1,8 µA (chỉ dòng tĩnh LDO) | Khi MCU chưa gửi lệnh quét | Datasheet ME6231 |
| Đang quét làm mới (Update) | **~8 mA** (đỉnh tức thời 15-20 mA) | 2 ~ 3 giây trong chu kỳ làm mới | Datasheet UC8253 & E037A75 |

## Cơ chế ngủ sâu và tiết kiệm pin

1. E-paper là công nghệ màn hình lưỡng ổn định (bi-stable): sau khi các hạt mực đen/trắng đã di chuyển vào vị trí, màn giữ nguyên hình ảnh vĩnh viễn mà không cần cấp điện.
2. Để tiết kiệm pin tối đa trên thiết bị bỏ túi:
   - Sau khi hoàn tất lệnh quét (`0x12 DRF`) và chân `BUSY` lên mức 1, phải tắt nguồn cao áp bằng lệnh `0x02` (Power OFF).
   - Tiếp tục đưa controller vào chế độ ngủ sâu bằng lệnh `0x07` kèm byte dữ liệu `0xA5`.
   - Khi ở trạng thái ngủ sâu, màn chỉ tiêu thụ khoảng 3 µA.
3. Khi cần vẽ tiếp: phát xung reset phần cứng trên chân RES (kéo thấp 50 µs rồi kéo cao) để đánh thức controller.
