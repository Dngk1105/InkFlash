# Thiết lập Figma cho pixel art

## Vấn đề

Panel 240 × 416, 1 bit mỗi pixel. Figma mặc định làm việc với vector và nửa pixel: chữ Inter làm mờ viền, fill gradient
và shadow không vẽ được trên panel. File Figma chưa cấu hình thì `tools/figma2ink.py` báo lỗi hoặc ảnh Figma lệch ảnh
host renderer.

## Cách xử lý

### Font

Firmware có năm font. Text style trong Figma đặt tên đúng tên font của `ui`.

| Text style | Font Figma | Cỡ | Line height | Nguồn |
|---|---|---|---|---|
| `unifont_vi_16` | `Unifont` | 16 | 16 | `tools/figma2ink/make_unifont_ttf.py` |
| `fixed_10` | `Fixed 6x10` | 10 | 10 | `tools/figma2ink/make_u8g2_ttf.py` từ `u8g2_font_6x10_tf` |
| `tiny5_8` | `Tiny5 Ink` | 14 | 14 | `tools/figma2ink/make_u8g2_ttf.py` từ `ink_ui_font_tiny5_8.c` |
| `tiny5_16` | `Tiny5 Ink` | 28 | 28 | như trên, 2× |
| `tiny5_24` | `Tiny5 Ink` | 42 | 42 | như trên, 3× |

- Cỡ khác (ví dụ 18) không có trong firmware: figma2ink báo lỗi "text style không có trong danh sách font của `ui`".
- Không dùng `Tiny5` của Google Fonts cho text style: Figma áp kerning của font đó (chuỗi có số, dấu cách hẹp hơn
  firmware 1–2 px, chữ bị cắt `…` trên panel) và đặt baseline theo metric của TTF (chữ cao hơn panel 1, 3, 4 px ở cỡ
  8, 16, 24). `Tiny5 Ink` sinh từ chính font firmware, không kerning, ascent 11 px và descent 3 px như firmware.
- Line height của `tiny5_*` là chiều cao glyph cao nhất (chữ có hai dấu chồng); muốn dòng sát hơn thì đặt line height
  nhỏ hơn trong Figma, figma2ink đổi chênh lệch thành `line_gap`.
- Đổi line height trên layer thì Figma tách layer khỏi text style. figma2ink khi đó nhận font theo cặp font Figma và cỡ
  trong bảng trên; cặp khác bảng thì báo lỗi.
- Cỡ chữ và line height là số nguyên, letter spacing 0, một style trong một text.
- Mỗi text style tạo một lần trong Figma (Local styles), layer text gắn style đó. Không đổi cỡ bằng tay trên layer.
- Cài `Unifont`: `python tools/figma2ink/make_unifont_ttf.py <unifont.otf> -o unifont-vi-18.0.01.ttf --install`, rồi
  khởi động lại `figma_agent.exe`.
- Cài `Fixed 6x10`: `python tools/figma2ink/make_u8g2_ttf.py u8g2_font_6x10_tf --family "Fixed 6x10" -o fixed-6x10.ttf
  --install`, rồi khởi động lại `figma_agent.exe`. Script giải mã chính mảng font mà firmware dùng, mỗi pixel thành một
  ô vuông, nên glyph trong Figma trùng bitmap trên panel.
- Cài `Tiny5 Ink`: `python tools/figma2ink/make_u8g2_ttf.py components/core/ink_ui/fonts/ink_ui_font_tiny5_8.c --family
  "Tiny5 Ink" -o tiny5-ink.ttf --install`, rồi khởi động lại `figma_agent.exe`.
- Hai script trên cần `fontTools` (`pip install fonttools`).

### Grid và toạ độ

1. Frame màn: `<app>_<màn>`, cỡ 240 × 416 (dọc) hoặc 416 × 240 (ngang).
2. Layout grid: Grid, size 1, để mọi layer nhảy theo pixel. Thêm layout grid cột 8 px cho layer động: display căn changed
   window theo 8 px.
3. View → Pixel Grid bật, View → Snap to pixel grid bật. Zoom từ 800 % trở lên thì thấy từng pixel.
4. Nudge: Preferences → Nudge amount `1` và `8` (Shift + mũi tên).
5. Mọi X, Y, W, H là số nguyên. Layer sau khi scale hoặc rotate dễ ra số lẻ: kiểm cột X, Y trong panel Design.
6. Line 1 px: dùng Rectangle cao hoặc rộng 1 px, không dùng Line.
7. Stroke canh trong (Inside).

### Màu và dither

| Mục | Quy tắc |
|---|---|
| Màu | Chỉ đen `#000000`, trắng `#FFFFFF`, xám R = G = B |
| Opacity | 100 % |
| Effect | Không shadow, blur |
| Chữ | Đen hoặc trắng, không xám |
| Fill xám | Hiện trên panel thành dither 4 × 4 Bayer neo tại gốc canvas (`ink_ui_fill()`); fill dither bỏ qua corner radius |

Tạo Local styles màu cho từng mức xám dùng trong thiết kế, đặt tên `gray_<level>` với `level` là giá trị R (0 đến 255).
Mức xám ra mẫu dither nào xem `fill_dither()` trong `components/core/ink_ui/ink_ui_canvas.c`.

Figma vẽ xám thật (liên tục), panel vẽ dither. Để xem đúng màn: chạy `python tools/render_view.py <view> --compare
figma.png`, hoặc xem `<output>_diff.pbm`. Khi cần xem ngay trong Figma, import ảnh PNG 1 bit của host renderer và đặt làm
reference layer.

### Vẽ pixel tay (icon, bitmap)

- Icon vẽ trên frame cỡ nguyên (ví dụ 16 × 16), Rectangle 1 × 1 cho từng pixel, hoặc vector có toạ độ nguyên và fill đen.
- Xuất PNG ở 1x: pixel có alpha từ 128 trở lên thành bit 1 (`INK_UI_BITMAP_FORMAT_MASK1`).
- Gom icon thành component trong một page riêng, đặt tên theo chức năng.
- Ảnh xám cần dither: dither ngoài Figma (Floyd–Steinberg hoặc Bayer) rồi import PNG 1 bit. figma2ink không nhận layer ảnh.

### Kiểm trước khi sinh view

1. Chạy `python tools/figma2ink.py <link> --app <app> --save build-render/snap`. Lỗi thì sửa Figma theo bảng "Luật kiểm"
   trong `figma-to-code.md`.
2. Chạy `python tools/render_view.py <view> --compare build-render/snap/figma.png`: số pixel lệch phải bằng 0 với màn
   chỉ có đen và trắng.
