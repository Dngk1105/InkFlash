# Màn e-paper WeAct 3.7"

Trạng thái chung: **Chưa đối chiếu datasheet.** Thông số dưới đây lấy từ thư viện GxEPD2 và demo `test/src/main.cpp`
(đang chạy được trên thiết bị với Arduino). Driver `components/drivers/drv_display/display_uc8253.c` viết theo đó.

## Thông số chính

| Thông số | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Panel | Good Display GDEY037T03 | GxEPD2 `GxEPD2_370_GDEY037T03.h`; demo `main.cpp` | Chưa đối chiếu datasheet |
| Chip điều khiển (controller) | UC8253 | GxEPD2 | Chưa đối chiếu datasheet |
| Độ phân giải native (px) | 240 (rộng) × 416 (cao) | GxEPD2 | Chưa đối chiếu datasheet |
| Độ phân giải khi xoay (rotation 1) | 416 × 240, nằm ngang | demo `setRotation(1)` | Đã dùng trong demo |
| Số màu | Đen/trắng, 1 bit/điểm | GxEPD2 `hasColor = false` | |
| Partial refresh | Có, vùng theo byte trên trục x native | GxEPD2 `hasPartialUpdate = true` | |
| Thời gian full refresh | Đo trên máy ngày 2026-10-21 (gồm cả truyền dữ liệu): sóng chuẩn 2,8 s; sóng nhanh 2,3 s. Thư viện ghi ~1,0 s cho riêng phần quét panel | Đo bằng demo InkFlash | Đã đo |
| Thời gian deep clean | 8,3 s (1 vòng đen → trắng, rồi vẽ) | Đo bằng demo InkFlash | Đã đo |
| Thời gian partial refresh | ~0,35 s | GxEPD2 `partial_refresh_time` | Cần đo bằng demo InkFlash |
| Điện áp cấp (VCC) | 3,3 V (cả nguồn lẫn tín hiệu) | Ghi chú đầu file GxEPD2 | |
| Dòng khi refresh / ngủ sâu | chưa có | | Chưa đo |
| Giao tiếp | SPI 4 dây, mode 0, 4 MHz, chỉ ghi (không MISO) | GxEPD2 | |
| BUSY | Mức thấp = đang bận | GxEPD2 `busy_level = LOW` | |

## Sơ đồ chân module → ESP32-S3 (breadboard)

Xem [pinmap.md](pinmap.md). Tên chân in trên module: SCL, SDA, CS, D/C, RES, BUSY.

| Chân module | GPIO | Ghi chú |
|---|---|---|
| SCL (SCK) | 4 | |
| SDA (MOSI) | 5 | |
| CS | 6 | |
| D/C | 7 | |
| RES | 15 | Reset phần cứng: cao 10 ms, thấp 50 ms, cao 50 ms |
| BUSY | 16 | Kéo lên nội |

## Chống bóng mờ và chấm lốm đốm

- **Sóng chuẩn** (mặc định) cho FULL: sạch hơn sóng nhanh, chậm hơn khoảng 0,5 s. Bật lại sóng nhanh bằng `INK_DISPLAY_UC8253_FAST_FULL`.
- **Deep clean** (`INK_REFRESH_CLEAN`): đảo cả màn đen rồi trắng trước khi vẽ. Lần xoá lúc khởi động cũng làm vậy.
  Số vòng đen → trắng đặt bằng `INK_DISPLAY_UC8253_CLEAN_CYCLES` (0–3, mặc định 1, mỗi vòng thêm khoảng 5,5 s).
- Làm mới cục bộ (partial) nhiều lần sẽ tích bóng mờ. Cứ vài chục lần nên deep clean một lần.

## Mô hình bộ nhớ ảnh và lệnh

Controller có hai vùng bộ nhớ ảnh: `0x13` = ảnh mới, `0x10` = ảnh cũ. Làm mới nhanh so sánh hai vùng, nên sau mỗi
lần làm mới driver ghi lại ảnh cũ bằng ảnh mới. Lần vẽ đầu tiên (hoặc sau ngủ sâu) driver xoá trắng toàn màn trước.

| Bước | Lệnh |
|---|---|
| Khởi tạo | `0x00` + `1E 0D` (soft reset), rồi `0x00` + `1F 0D` |
| Ghi vùng ảnh | `0x91` (vào partial), `0x90` + cửa sổ (x, xe, y, ye, `01`), `0x13` hoặc `0x10` + dữ liệu, `0x92` (ra partial) |
| Làm mới toàn phần | `E0 02`, `E5 5A`, `50 97`, `04` (bật nguồn), `12` (refresh), `02` (tắt nguồn) |
| Làm mới cục bộ | `E0 02`, `E5 6E`, `50 D7`, `04`, `12`, `02` |
| Ngủ sâu | `07` + `A5` |

Nguồn: `GxEPD2_370_GDEY037T03.cpp`. Cần đối chiếu với datasheet UC8253.

## Ảnh module

Lưu ảnh mặt trước và mặt sau vào `docs/hardware/img/` khi có.
