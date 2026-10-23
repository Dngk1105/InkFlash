# ADR 0001: Viết firmware bằng C11

- **Ngày:** 2026-10-23
- **Trạng thái:** Đã chốt

## Bối cảnh

Firmware có nhiều tầng, cần đổi được driver mà không sửa logic, và người phát triển muốn tự tối ưu tầng dưới.
Ứng viên: C, C++, hoặc trộn (C cho tầng dưới, C++ cho app).

## Quyết định

Toàn bộ firmware viết bằng C11. Interface giữa các tầng là `struct` chứa con trỏ hàm cộng một con trỏ `ctx`.

## Lý do

- ESP-IDF, FreeRTOS, LittleFS đều là C: gọi thẳng, backtrace và log đọc được ngay khi debug.
- `struct` con trỏ hàm đủ để thay driver mà không cần kế thừa. Flipper Zero (`furi_hal`) và Zephyr đều dùng cách này.
- Tối ưu driver (DMA, căn byte, đặt hàm trong IRAM) dễ kiểm soát khi không có constructor ẩn, exception, RTTI.
- Domain và UI viết bằng C thuần biên dịch được bằng gcc trên PC để test.

## Hệ quả

- Không có class: quy ước đặt tên tiền tố (`ink_`, `svc_`, `drv_<chip>_`) để tránh trùng tên.
- Hàm trả `esp_err_t` (dùng mã lỗi có sẵn của ESP-IDF).
- Nếu tầng UI về sau quá phức tạp, có thể cân nhắc C++ riêng cho `ui/` mà không ảnh hưởng tầng khác.
