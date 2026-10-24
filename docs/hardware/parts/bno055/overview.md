# BNO055

Cảm biến chuyển động 9 trục tích hợp vi xử lý hợp nhất dữ liệu cảm biến (Sensor Fusion), giao tiếp I2C

| Thông số                               | Giá trị               | Ghi chú                                   |
| ---------------------------------------- | ----------------------- | ------------------------------------------ |
| Điện áp cấp nguồn ($V_{DD}$)      | 2,4 V – 3,6 V          | Cấp từ nguồn 3,3 V                      |
| Điện áp giao tiếp I/O ($V_{DDIO}$) | 1,7 V – 3,6 V          | Tương thích logic 3,3 V                 |
| Dòng tiêu thụ chế độ NDOF          | 12,3 mA                 | Chạy thuật toán hợp nhất 9 trục BSX3 |
| Dòng tiêu thụ chế độ Suspend       | 40 µA                  | Chế độ tạm dừng                       |
| Tốc độ xung nhịp I2C                 | Tối đa 400 kHz        | Chuẩn Fast Mode                           |
| Địa chỉ I2C (7-bit)                   | `0x28` hoặc `0x29` | Tùy thuộc chân `COM3`                 |

## Cấu hình chân & Địa chỉ I2C

- **Địa chỉ `0x28`:** Chân `COM3` nối mass (GND) — mặc định trên đa số module breakout.
- **Địa chỉ `0x29`:** Chân `COM3` nối lên nguồn 3,3 V.
- **Chọn giao diện I2C:** Nối cả hai chân `PS0` và `PS1` xuống mass (GND).

## Các chế độ hoạt động chính

- **CONFIGMODE:** Chế độ cấu hình thanh ghi sau khi khởi động hoặc reset.
- **NDOF (9 trục):** Hợp nhất dữ liệu Gia tốc, Con quay hồi chuyển và Từ kế để xuất góc Euler và Quaternion tuyệt đối không bị trôi trôi dạt.
- **IMU (6 trục):** Chỉ hợp nhất Gia tốc và Con quay hồi chuyển, bỏ qua từ trường (dùng khi môi trường xung quanh có nhiều vật liệu nhiễm từ).
