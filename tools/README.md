# InkFlash Tools

Thư mục chứa các công cụ phát triển phần cứng và phần mềm của dự án InkFlash.

## Danh sách công cụ

- `new.py`: tạo khối mới từ template (`python tools/new.py core <tên> --design docs/design/<file>.md`).
- `check.py`: kiểm tra quy ước, API cấm, định dạng trước khi commit (`python tools/check.py`).
- `profile_gen.py`: kiểm và sinh danh sách profile bảng sóng của driver màn từ `components/drivers/ink_drv_uc8253/profiles/profiles.json` (`python tools/profile_gen.py`; `--list` in danh sách; `--blob <tên> -o <file>.inkw` gói một profile cho firmware test ở nhánh `bench`).
- `fix_compile_commands.py`: xoá cờ `-m*`, `-f*` khỏi `build/compile_commands.json` để clangd hết báo "Unknown argument"; `idf.py build` tự chạy nó.
- `render_view.py`: host renderer, vẽ một view `.gen.c` trên PC ra PNG hoặc PBM để so với Figma (`python tools/render_view.py example_view`; xem `docs/design/figma-to-code.md`).
- `figma2ink.py`: công cụ sinh view, đọc một frame Figma (link hoặc snapshot) rồi ghi `.gen.c`, `.gen.h`, `.sample.json` (`python tools/figma2ink.py <link> --app <app> --save <thư mục>`; cần `FIGMA_TOKEN` khi đọc link; xem `docs/design/figma-to-code.md`). `figma2ink/make_unifont_ttf.py` tạo font Unifont TTF cho máy thiết kế.
- `u8g2_font.py`: chép một font của U8g2 ra file `.c` trong `components/core/ink_ui/fonts/`.
- `build_all.py`: chạy check, host test, host renderer với view mẫu, sinh và vẽ view từ snapshot Figma mẫu, build `templates/` và firmware (chạy trong terminal ESP-IDF).
- `vision_test/`: bộ công cụ kiểm thử thị giác thời gian thực cho màn hình e-ink từ camera iPhone qua cáp Lightning hoặc Webcam. Xem chi tiết tại `tools/vision_test/README.md`.
