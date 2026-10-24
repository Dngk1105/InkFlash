# Module e-paper WeAct 3.7"

Module màn hình e-paper 3,7 inch của WeAct Studio, sử dụng panel E037A75-A0 và chip điều khiển tích hợp UC8253. Đã đối chiếu toàn diện với bộ datasheet gốc và sơ đồ nguyên lý phần cứng của WeAct Studio. Đã chốt cho sản phẩm máy học flashcard InkFlash.

Cột "Trạng thái": `Đã xác nhận` (đã đo trên máy hoặc đã đối chiếu datasheet).

## Bảng thông số kỹ thuật cốt lõi

| Thông số | Giá trị | Nguồn kiểm chứng | Trạng thái |
|---|---|---|---|
| **Mã Panel** | Yingruida E037A75-A0 (Tương thích GDEY037T03) | Datasheet E037A75 & Sơ đồ WeAct | **Đã xác nhận** |
| **Controller IC** | UltraChip UC8253 (Driver + Timing Controller) | Datasheet UC8253 | **Đã xác nhận** |
| **Độ phân giải gốc** | 240 (ngang) × 416 (dọc) pixels (129 DPI) | Datasheet E037A75 | **Đã xác nhận** |
| **Độ phân giải xoay ngang** | 416 × 240 pixels | Demo Arduino & InkFlash UI | **Đã xác nhận** |
| **Kích thước hiển thị (AA)** | 47,04 mm (ngang) × 81,54 mm (dọc) | Bản vẽ cơ khí WeAct | **Đã xác nhận** |
| **Kích thước ngoài module** | 53,00 mm × 92,99 mm × 0,95 mm | Bản vẽ cơ khí WeAct | **Đã xác nhận** |
| **Số màu** | Đen / Trắng (1 bit mỗi điểm ảnh) | Datasheet E037A75 & UC8253 | **Đã xác nhận** |
| **Điện áp hoạt động ($V_{DD}$)** | 3,3 V chuẩn (dải cho phép 2,4 V ~ 3,6 V) | Sơ đồ nguyên lý & Datasheet ME6231 | **Đã xác nhận** |
| **Chuẩn giao tiếp** | SPI 4 dây (Mode 0), xung nhịp tối đa 20 MHz, chỉ ghi | Datasheet UC8253 | **Đã xác nhận** |
| **Logic chân BUSY** | **Mức 0 = Đang bận; Mức 1 = Rảnh (cho phép gửi lệnh)** | Datasheet UC8253 (mục BUSY_N) & code mẫu WeAct | **Đã xác nhận** |
| **Partial refresh** | Hỗ trợ; vùng làm mới căn theo byte trên trục X | Datasheet UC8253 (Lệnh 0x90/0x91) | **Đã xác nhận** |
| **Dòng điện ngủ sâu** | ~3 µA (chỉ panel) / ~4,8 µA (gồm LDO) | Đo kiểm & Datasheet UC8253 | **Đã xác nhận** |

## Điều hướng tài liệu chuyên đề

Mỗi đặc tính và tri thức chuyên sâu của màn hình được tách biệt vào một file riêng:

| File tài liệu | Nội dung chuyên đề | Vấn đề cần tra cứu |
|---|---|---|
| **[`pinout.md`](pinout.md)** | Sơ đồ chân & giao tiếp phần cứng | Thứ tự 8 chân header, chân nối sang ESP32-S3, cấu hình cầu hàn SB1–SB5, mạch booster cao áp. |
| **[`commands.md`](commands.md)** | Bộ nhớ ảnh & lệnh controller | Cấu trúc 2 buffer SRAM (12.480 byte), bảng mã lệnh điều khiển UC8253 (`0x00`–`0x92`), trình tự khởi tạo. |
| **[`refresh.md`](refresh.md)** | Cơ chế làm mới & chất lượng ảnh | Thời gian quét các chế độ (Full, Fast, Partial, Deep clean), khử bóng mờ (ghosting), giới hạn nhiệt độ. |
| **[`power.md`](power.md)** | Nguồn điện & tiết kiệm pin | Dải điện áp, dòng tiêu thụ từng trạng thái, quy trình tắt nguồn cao áp và đưa vào Deep Sleep. |
