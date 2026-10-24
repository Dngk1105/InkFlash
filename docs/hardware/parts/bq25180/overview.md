# BQ25180

IC quản lý sạc pin LiPo và điều phối nguồn (Power Path), giao tiếp I2C

| Thông số                                | Giá trị        | Ghi chú                                                  |
| ----------------------------------------- | ---------------- | --------------------------------------------------------- |
| Điện áp vào hoạt động ($V_{IN}$) | 3,0 V – 5,9 V   | Qua cổng USB Type-C                                      |
| Chịu áp đầu vào tối đa             | 25 V             | Bảo vệ sốc áp cổng sạc                              |
| Điện áp ngắt sạc quá áp (OVP)      | 6,5 V            | Tự ngắt sạc khi quá áp                               |
| Điện áp sạc đầy ($V_{BAT\_REG}$)  | 4,20 V           | Cấu hình qua I2C từ 3,60 V đến 4,65 V (bước 10 mV) |
| Dòng sạc pin ($I_{CHG}$)              | 5 mA – 1000 mA  | Lập trình qua thanh ghi I2C                             |
| Dòng rò chế độ Ship mode             | ~10 nA           | Ngắt BATFET, bảo toàn pin lưu kho                     |
| Dòng tĩnh khi chạy pin ($I_{BAT}$)   | ~5 µA           | Không cắm sạc, chưa vào Ship mode                    |
| Địa chỉ I2C                            | `0x6B` (7-bit) | Tốc độ tối đa 1 MHz                                  |

## Điều phối nguồn (Power Path)

- Khi cắm USB: IC ưu tiên lấy nguồn từ USB nuôi toàn bộ hệ thống qua đường SYS, dòng thừa dùng để sạc pin.
- Khi tải tăng đột biến: Nếu dòng tiêu thụ của hệ thống vượt quá dòng cấp từ USB, pin sẽ tự động phụ tải (Dynamic Power Management).
- Khi rút USB: Pin lập tức cấp nguồn cho đường SYS qua khóa chuyển mạch BATFET tích hợp, không làm reset hệ thống.

## Chế độ Ship mode

Ship mode ngắt hoàn toàn pin khỏi đường tải SYS, đưa dòng rò xuống mức nano-ampe (~10 nA), giữ pin không bị cạn kiệt khi lưu kho dài ngày.

- **Vào Ship mode:** Ghi thanh ghi I2C set bit `EN_RST_SHIP = 0b01` (thanh ghi `0x09`), hoặc nhấn giữ chân `TS/MR` đủ thời gian cấu hình.
- **Thoát Ship mode (Bật nguồn):**
  1. Cắm sạc USB: Điện áp chân `IN` vượt ngưỡng UVLO, IC tự động đóng khóa BATFET và cấp nguồn lại cho toàn mạch.
  2. Nhấn giữ nút nguồn tại chân `TS/MR` trong khoảng 1 giây.
