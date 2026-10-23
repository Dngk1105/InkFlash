# ADR 0003: Cho phép cấp phát động, có quy tắc

- **Ngày:** 2026-10-23
- **Trạng thái:** Đã chốt

## Bối cảnh

Kế hoạch ban đầu cấm `malloc` sau khi khởi động để tránh rò rỉ và phân mảnh heap. Cách này an toàn nhưng tốn công:
mọi buffer phải tính trước kích thước tối đa.

## Quyết định

Được dùng `malloc` / `heap_caps_malloc` lúc chạy, theo các quy tắc dưới.

## Quy tắc

- **Cấm** cấp phát trong ISR, trong vòng lặp mỗi frame của render, trong vòng lặp mỗi khối của audio.
  Các đường nóng này dùng buffer cấp một lần lúc service khởi động.
- **Được** cấp phát khi mở app/scene, khi nạp deck, khi tạo yêu cầu.
- **App** cấp phát qua `app_alloc()` để app manager thu hồi phần rò rỉ khi app dừng (ADR 0004).
- **Chọn vùng nhớ:** buffer DMA (SPI, I2S) dùng `MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL`; dữ liệu lớn (deck, font,
  buffer phụ) dùng `MALLOC_CAP_SPIRAM`.

## Giám sát

- Lệnh console `heap`: RAM nội còn trống, mức thấp nhất, khối liền lớn nhất (nhỏ dần nghĩa là phân mảnh), DMA, PSRAM.
- `CONFIG_HEAP_POISONING_LIGHT` bật khi phát triển để bắt lỗi ghi tràn heap.
