# Module e-paper WeAct 3.7"

Panel Good Display GDEY037T03, controller UC8253. Thông số lấy từ thư viện GxEPD2 và bản demo Arduino (chạy được trên
máy), **chưa đối chiếu datasheet**.

Cột "Trạng thái": `Đã xác nhận` (đã đo trên máy hoặc đã đối chiếu datasheet) hoặc `Chưa xác nhận`.

| Thông số | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Panel | Good Display GDEY037T03 | GxEPD2 `GxEPD2_370_GDEY037T03.h` | Chưa xác nhận |
| Controller | UC8253 | GxEPD2 | Chưa xác nhận |
| Độ phân giải gốc | 240 (rộng) × 416 (cao) | GxEPD2 | Chưa xác nhận |
| Độ phân giải khi xoay ngang | 416 × 240 | Demo `setRotation(1)` | Đã xác nhận |
| Số màu | Đen/trắng, 1 bit mỗi điểm | GxEPD2 `hasColor = false` | Chưa xác nhận |
| Partial refresh | Có; vùng làm mới căn theo byte trên trục x gốc | GxEPD2 `hasPartialUpdate = true` | Chưa xác nhận |
| Điện áp cấp | 3,3 V cho cả nguồn lẫn tín hiệu | Ghi chú đầu file GxEPD2 | Chưa xác nhận |
| Giao tiếp | SPI 4 dây, mode 0, 4 MHz, chỉ ghi (không có MISO) | GxEPD2 | Chưa xác nhận |
| BUSY | Mức thấp = đang bận | GxEPD2 `busy_level = LOW` | Chưa xác nhận |

Tên chân in trên module: SCL (SCK), SDA (MOSI), CS, D/C, RES, BUSY. Chân RES: cao 10 ms, thấp 50 ms, cao 50 ms để
reset phần cứng. Chân BUSY dùng kéo lên trong.

Thời gian làm mới và chống bóng mờ: `refresh.md`. Bộ nhớ ảnh và lệnh controller: `commands.md`. Cùng thư mục.
