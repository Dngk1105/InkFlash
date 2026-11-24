# Font lưu trữ

| File | Family trong Figma | Dùng cho text style | Nguồn | Giấy phép |
|---|---|---|---|---|
| `figma/unifont-vi-18.0.01.ttf` | Unifont | `unifont_vi_16` | GNU Unifont 18.0.01, qua `tools/figma2ink/make_unifont_ttf.py` | OFL 1.1 hoặc GPL 2+ có Font Embedding Exception |
| `figma/fixed-6x10.ttf` | Fixed 6x10 | `fixed_10` | `u8g2_font_6x10_tf`, qua `tools/figma2ink/make_u8g2_ttf.py` | `third_party/u8g2/LICENSE` |
| `figma/tiny5-ink.ttf` | Tiny5 Ink | `tiny5_8`, `tiny5_16`, `tiny5_24` | `ink_ui_font_tiny5_8.c`, qua `tools/figma2ink/make_u8g2_ttf.py` | OFL 1.1, `tiny5/OFL.txt` |
| `tiny5/Tiny5-VF.ttf` | Tiny5 (không dùng trong Figma) | | Google Fonts `ofl/tiny5`, font variable gốc, giữ để tham khảo | OFL 1.1, `tiny5/OFL.txt` |

Cài trên máy mới: chuột phải từng file trong `figma/` → Install, rồi thoát hẳn và mở lại Figma. Cỡ chữ và line height:
`docs/design/figma-setup.md`.
