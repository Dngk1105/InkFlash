# ADR 0002: Event bus dùng esp_event

- **Ngày:** 2026-10-23
- **Trạng thái:** Đã chốt

## Bối cảnh

Các khối cần báo cho nhau "đã xảy ra X" (nút được bấm, có intent mới, màn vẽ xong) mà không gọi thẳng nhau.
Ban đầu kế hoạch đề xuất tự viết bus có queue kích thước cố định, vì `esp_event` cấp phát bộ nhớ mỗi lần post.

## Quyết định

Dùng `esp_event` với một event loop riêng (`ink_bus`, không dùng default loop), bọc trong `components/core/ink_bus.c`.

## Lý do

- Không phải tự viết và bảo trì bus; có sẵn đăng ký/huỷ đăng ký và post từ ISR.
- Mỗi lần post, `esp_event` chép dữ liệu sự kiện vào heap rồi giải phóng sau khi xử lý. Với vài sự kiện mỗi giây
  và dữ liệu vài chục byte, chi phí này không đáng kể so với 8 MB PSRAM.

## Hệ quả và quy tắc

- Bus chỉ để **thông báo**. Ra lệnh cho service thì gọi hàm API của service đó.
- Mọi handler chạy tuần tự trong task của loop, nên handler chỉ chép sự kiện vào queue của service (`ink_service.c`).
- `esp_event` không đưa kích thước dữ liệu cho handler, nên mỗi sự kiện mới phải thêm vào `ink_bus_evt_size()`.
- Post thất bại (queue của loop đầy) được ghi log và đếm (`ink_bus_drop_count()`, lệnh `status`), không bỏ qua im lặng.
- Dữ liệu sự kiện giữ nhỏ (≤ 24 byte, kích thước ô dữ liệu của message service).
