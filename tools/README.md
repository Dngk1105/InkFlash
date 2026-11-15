# InkFlash Tools

Thư mục chứa các công cụ phát triển phần cứng và phần mềm của dự án InkFlash.

## Danh sách công cụ

- `new.py`: tạo khối mới từ template (`python tools/new.py core <tên> --design docs/design/<file>.md`).
- `check.py`: kiểm tra quy ước, API cấm, định dạng trước khi commit (`python tools/check.py`).
- `profile_gen.py`: kiểm và sinh danh sách profile bảng sóng của driver màn từ `components/drivers/ink_drv_uc8253/profiles/profiles.json` (`python tools/profile_gen.py`; `--list` in danh sách; `--blob <tên> -o <file>.inkw` gói một profile cho bench).
- `fix_compile_commands.py`: xoá cờ `-m*`, `-f*` khỏi `build/compile_commands.json` để clangd hết báo "Unknown argument"; `idf.py build` tự chạy nó.
- `build_all.py`: chạy check, host test, build `templates/`, firmware và firmware bench (chạy trong terminal ESP-IDF).
- `display_bench/`: công cụ test panel (`docs/design/display/bench.md`). Nạp firmware bench: `idf.py -C tools/display_bench/firmware -p COM4 flash`; mở giao diện: `python tools/display_bench/bench_server.py` rồi vào `http://localhost:8253` trên Chrome hoặc Edge. Test xong nạp lại firmware chính: `idf.py -p COM4 flash`.
- `vision_test/`: bộ công cụ kiểm thử thị giác thời gian thực cho màn hình e-ink từ camera iPhone qua cáp Lightning hoặc Webcam. Xem chi tiết tại `tools/vision_test/README.md`.
