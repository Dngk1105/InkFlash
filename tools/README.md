# InkFlash Tools

Thư mục chứa các công cụ phát triển phần cứng và phần mềm của dự án InkFlash.

## Danh sách công cụ

- **[`datasheet_processor/`](datasheet_processor/README.md)**: Công cụ bóc tách tài liệu kỹ thuật PDF (datasheet) thành Markdown sạch sử dụng IBM Docling và DeepSheet.
- `new.py`: tạo khối mới từ template (`python tools/new.py core <tên> --design docs/design/<file>.md`).
- `check.py`: kiểm tra quy ước, API cấm, định dạng trước khi commit (`python tools/check.py`).
- `build_all.py`: chạy check, host test, build `templates/` và firmware (chạy trong terminal ESP-IDF).
