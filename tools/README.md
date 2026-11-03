# InkFlash Tools

Thư mục chứa các công cụ phát triển phần cứng và phần mềm của dự án InkFlash.

## Danh sách công cụ

- `new.py`: tạo khối mới từ template (`python tools/new.py core <tên> --design docs/design/<file>.md`).
- `check.py`: kiểm tra quy ước, API cấm, định dạng trước khi commit (`python tools/check.py`).
- `font_gen.py`: đổi file TTF thành font 1 bit cho `ink_ui` (`python tools/font_gen.py <file.ttf> --size 24 --name <tên> --license "<giấy phép>"`; cần Pillow).
- `profile_gen.py`: kiểm và sinh danh sách profile bảng sóng của driver màn từ `components/drivers/ink_drv_uc8253/profiles/profiles.json` (`python tools/profile_gen.py`; `--list` in danh sách; `--blob <tên> -o <file>.inkw` gói một profile cho firmware test ở nhánh `bench`).
- `fix_compile_commands.py`: xoá cờ `-m*`, `-f*` khỏi `build/compile_commands.json` để clangd hết báo "Unknown argument"; `idf.py build` tự chạy nó.
- `build_all.py`: chạy check, host test, build `templates/` và firmware (chạy trong terminal ESP-IDF).
