# InkFlash

Máy học flashcard bỏ túi: ESP32-S3, màn e-ink, 2 nút, chạy pin. ESP-IDF v6.1 + FreeRTOS.
Bản cũ: tag `skeleton-v2`, `demo-v0`.

## Đọc gì

Không đọc hết `docs/`. Bắt đầu từ `docs/architecture.md`: toàn cảnh, rồi bảng "Gặp vấn đề thì đọc file nào".

## Luật

- Repo được AI hỗ trợ viết khi dev yêu cầu; dev làm duyệt và tối ưu.
- Code tuân thủ yêu cầu trong docs. Thấy code lệch docs hoặc lệch template: cảnh báo người làm và viết lại cho khớp.
- Kiến trúc chốt cùng người làm từng phần. Chưa chốt thì bàn trong chat, không ghi vào docs.
- Thông số phần cứng chưa có trong `docs/hardware/` thì hỏi, không đoán.
- Viết code ESP-IDF: chép từ `templates/`, không viết API theo trí nhớ. Tra thêm: docs offline v6.1 → MCP
  `espressif-docs` (bản mới nhất, phải đối chiếu lại v6.1) → web.
- Tạo khối mới bằng `tools/new.py`; `tools/check.py` phải qua trước khi báo xong.
- Build, flash: MCP `esp-idf` (`build_project`, `flash_project`).
- Viết hoặc sửa docs: đọc `docs/writing-docs.md` trước.
