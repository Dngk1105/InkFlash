# InkFlash Tools

Thư mục chứa các công cụ phát triển phần cứng và phần mềm của dự án InkFlash.

## Danh sách công cụ

- `new.py`: tạo khối mới từ template (`python tools/new.py core <tên> --design docs/design/<file>.md`).
- `check.py`: kiểm tra quy ước, API cấm, định dạng trước khi commit (`python tools/check.py`).
- `font_gen.py`: đổi file TTF thành font 1 bit cho `ink_ui` (`python tools/font_gen.py <file.ttf> --size 24 --name <tên> --license "<giấy phép>"`; cần Pillow).
- `fix_compile_commands.py`: xoá cờ `-m*`, `-f*` khỏi `build/compile_commands.json` để clangd hết báo "Unknown argument"; `idf.py build` tự chạy nó.
- `build_all.py`: chạy check, host test, build `templates/`, firmware và firmware test màn (chạy trong terminal ESP-IDF).
- `display_bench/`: công cụ test màn (`docs/design/display/bench.md`). Nạp firmware test: `idf.py -C tools/display_bench/firmware -p COM4 flash`; mở giao diện: `python tools/display_bench/bench_server.py` rồi vào `http://localhost:8253` trên Chrome hoặc Edge. Test xong nạp lại firmware chính: `idf.py -p COM4 flash`.
