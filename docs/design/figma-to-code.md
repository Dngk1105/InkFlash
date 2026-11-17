# Figma → code

## Vấn đề

Màn của app được thiết kế trong Figma. Chép toạ độ, font, cỡ chữ từ Figma sang C bằng tay thì chậm, dễ lệch vài pixel,
và mỗi lần sửa thiết kế phải chép lại. Panel chỉ có 240 × 416 pixel, 1 bit mỗi pixel, nên lệch một pixel cũng thấy và
cộng debt cho những pixel đổi thừa.

Ví dụ: màn danh sách bộ thẻ có thanh tiêu đề đen, tên bộ thẻ và các hàng tên bộ thẻ. Người thiết kế dời thanh tiêu đề
xuống 8 px trong Figma. Code phải đổi theo mà không ai sửa tay file C.

## Cách xử lý

### Một màn Figma là một view sinh lại, không sửa tay

View là hàm C vẽ một màn Figma lên canvas. Công cụ sinh view đọc Figma rồi ghi hai file vào thư mục của app:

```text
components/apps/ink_app_<tên>/
  scene_list.c           viết tay: command, scene state, chọn intent, acquire, submit
  views/list_view.gen.c  sinh từ Figma
  views/list_view.gen.h  sinh từ Figma: struct dữ liệu và prototype
  views/list_view.sample.json  sinh từ Figma: dữ liệu mẫu cho host renderer
```

- File `.gen.c` và `.gen.h` luôn sinh lại từ Figma, không sửa tay. Dòng đầu file ghi nguồn Figma và hash của phần còn
  lại; `tools/check.py` báo lỗi khi hash không khớp.
- Sửa thiết kế thì sửa Figma rồi chạy lại công cụ sinh view.
- Hành vi (command, scene state, intent, submit) nằm trong file scene viết tay. View chỉ vẽ.

Hàm `draw` của scene điền struct dữ liệu rồi gọi view:

```c
static void list_draw(const uint8_t *state) {
    const list_view_data_t data = {.title = "Bộ thẻ", .rows_count = 3 /* ... */};
    /* acquire, bind canvas */
    list_view_draw(canvas, &data);
    /* submit với intent PAGE */
}
```

### View là hàm C gọi `ink_ui`

View gọi thẳng các họa cụ của `ui` (`ink_ui_rect_split`, `ink_ui_fill`, `ink_ui_text_draw`...). Không có bảng layout
hay bộ đọc chạy trên máy.

- Đọc và debug được như code viết tay.
- Host renderer chỉ cần biên dịch view cùng `ink_ui` là ra ảnh để so với Figma.
- View không acquire, không submit, không giữ state.

### Dữ liệu động khai báo bằng tên layer

Layer có tên theo quy ước dưới đây thành field của struct `<view>_data_t` trong `.gen.h`. Layer khác là phần tĩnh, vẽ
y như Figma.

| Tên layer | Loại layer | Field sinh ra | Ví dụ |
|---|---|---|---|
| `{tên}` | Text | `const char *tên` | `{front}` → `const char *front` |
| `?tên` | Bất kỳ | `bool show_tên`; false thì không vẽ layer đó | `?empty_hint` → `bool show_empty_hint` |
| `*tên` | Frame Auto Layout chứa các hàng giống nhau | Mảng struct của một hàng, cỡ bằng số hàng vẽ trong Figma, kèm `uint8_t tên_count` | `*rows` có 6 hàng `{name}` → `rows[6]`, `rows_count` |

Hàng đầu tiên của `*tên` là hàng mẫu; các hàng còn lại trong Figma chỉ để đếm số hàng tối đa.

Tên sinh ra cho `*tên` trong view `list_view`:

- Struct một hàng: `list_view_<tên>_row_t`, ví dụ `list_view_rows_row_t`.
- Số hàng tối đa: macro `LIST_VIEW_<TÊN>_MAX`, ví dụ `LIST_VIEW_ROWS_MAX`. Scene dùng macro này để không ghi quá mảng.

### Scene chọn intent, view chia part

Figma không ghi intent. Intent là việc của hành vi: cùng một view có thể vẽ bằng `PAGE` khi mở scene và bằng `TEXT` khi
lật thẻ.

Frame con cấp một của màn có tên `part:<tên>` sinh thêm hàm `<view>_draw_<tên>()` bên cạnh `<view>_draw()`. Scene dùng
các hàm part để vẽ hai nhịp.

Ví dụ màn thẻ có `part:card` (viền thẻ) và `part:text` (chữ):

1. Scene vẽ `card_view_draw_card()`, submit với intent `UI`.
2. Report của frame đó tới main queue.
3. Scene vẽ `card_view_draw_text()` vào cùng frame, submit với intent `TEXT`.

### Bảng ánh xạ Figma → `ink_ui`

| Figma | `ink_ui` |
|---|---|
| Frame Auto Layout dọc / ngang | `ink_ui_rect_split()` theo `INK_UI_AXIS_COLUMN` / `INK_UI_AXIS_ROW` |
| Con Fixed | `ink_ui_length_t.fixed` = cỡ trong Figma |
| Con Fill | `ink_ui_length_t.flex` = 1 |
| Con Hug | `ink_ui_length_t.fixed` = cỡ Figma tính sẵn |
| Item spacing | Tham số `gap` của `ink_ui_rect_split()` |
| Padding trái = phải và trên = dưới | `ink_ui_rect_inset()` |
| Padding lệch | Rect tính sẵn bằng số |
| Căn con trong frame | `ink_ui_rect_align()` |
| Layer đặt tự do (không Auto Layout) | Rect tính sẵn, tương đối với frame cha |
| Rectangle có fill | `ink_ui_fill()`, corner radius → `radius` |
| Stroke canh trong | `ink_ui_stroke()`, stroke weight → `thickness` |
| Line | `ink_ui_line()` |
| Text | `ink_ui_text_draw()`: text style → font, căn trái/giữa/phải → `align`, số dòng tối đa và cắt `…` → `max_lines`, line height → `line_gap` |
| Vector, icon | Ảnh 1 bit `INK_UI_BITMAP_FORMAT_MASK1`, `static const` trong `.gen.c`, vẽ bằng `ink_ui_bitmap()` |
| Màu đen, trắng | `INK_UI_PAINT_BLACK`, `INK_UI_PAINT_WHITE` |
| Màu xám (R = G = B) | `INK_UI_PAINT_GRAY(level)`, `level` = giá trị R |

Text style trong Figma đặt tên đúng tên font của `ui`: `unifont_vi_16` → `ink_ui_font_unifont_vi_16`, `fixed_10` →
`ink_ui_font_fixed_10`. Máy thiết kế cài font GNU Unifont để Figma hiện gần đúng chữ trên panel.

### Luật kiểm

Công cụ sinh view kiểm Figma trước khi sinh. Lỗi thì dừng, không ghi file. Cảnh báo thì vẫn sinh và in ra.

| Luật | Mức | Lý do |
|---|---|---|
| Frame của màn cỡ 240 × 416 (dọc) hoặc 416 × 240 (ngang) | Lỗi | Cỡ panel |
| Text style không có trong danh sách font của `ui` | Lỗi | Firmware không có font đó |
| Màu không phải xám (R, G, B khác nhau), gradient, opacity dưới 100 %, effect (shadow, blur) | Lỗi | Panel 1 bit không vẽ được |
| Chữ màu xám | Lỗi | Chữ xám bị làm thành đen hoặc trắng theo ngưỡng 50 % |
| Stroke canh giữa hoặc canh ngoài | Lỗi | `ink_ui_stroke()` chỉ vẽ vào trong |
| Text `{tên}` để Hug | Lỗi | Cỡ chữ động không biết trước lúc sinh |
| Mép trái, mép phải của layer động (`{}`, `?`, `*`) không trên bội của 8 px | Cảnh báo | Display căn changed window theo 8 px |
| Fill xám có corner radius | Cảnh báo | Fill dither bỏ qua radius |
| Stroke, line màu xám | Cảnh báo | Thành đen hoặc trắng theo ngưỡng 50 % |

### Host renderer vẽ view trên PC

Host renderer là công cụ chạy trên PC: biên dịch một view cùng `ink_ui` thật bằng gcc, vẽ vào frame K1 240 × 416, rồi
ghi ra ảnh. Người làm so ảnh này với ảnh chụp từ Figma mà không cần nạp board.

Ví dụ, vẽ view mẫu của template app:

```text
python tools/render_view.py example_view                     → build-render/example_view.png
python tools/render_view.py example_view --part body         → build-render/example_view_body.png
python tools/render_view.py example_view --data d.json --scale 2 -o out.png
```

Lệnh chạy trong terminal ESP-IDF, vì cần `gcc`, `cmake`, `ninja` trong PATH.

**Đầu vào**

| Tham số | Ý nghĩa |
|---|---|
| `<view>` | Tên view, ví dụ `example_view`. Renderer tìm `<view>.gen.h` trong `components/apps/*/views/` và `templates/components/*/views/`. Trùng tên thì truyền đường dẫn file `.gen.h`. |
| `--data <file.json>` | Dữ liệu điền struct `<view>_data_t`. Không truyền thì dùng `<view>.sample.json` cạnh file `.gen.h`. |
| `--part <tên>` | Chỉ gọi `<view>_draw_<tên>()` lên nền trắng. Không truyền thì gọi `<view>_draw()`. |
| `--scale N` | Phóng mỗi pixel thành N × N. Mặc định 1. |
| `-o <file>` | Đuôi `.png` hoặc `.pbm`. Mặc định `build-render/<view>[_<part>].png`. |

File JSON là một object, mỗi key là tên field của struct, theo đúng quy ước tên layer:

```json
{"title": "Bộ thẻ", "rows": [{"name": "Tiếng Anh"}, {"name": "Hán tự"}], "show_empty_hint": false}
```

- Chuỗi JSON thành chuỗi C (UTF-8), `true`/`false` thành `bool`, `null` thành `nullptr`.
- Mảng `rows` tự điền thêm `rows_count` bằng số phần tử, trừ khi JSON đã ghi `rows_count`.
- Key không có trong struct hoặc mảng dài hơn `<VIEW>_<TÊN>_MAX` thì gcc báo lỗi, renderer in lỗi đó rồi dừng.

File `<view>.sample.json` do công cụ sinh view ghi, lấy từ nội dung đang có trong Figma: chữ trong layer `{tên}`, các
hàng của `*tên`, và layer `?tên` đang hiện hay ẩn. Vì vậy ảnh vẽ bằng dữ liệu mẫu phải khớp ảnh chụp Figma.

**Đầu ra**

| Định dạng | Pixel |
|---|---|
| PNG | Xám 1 bit, 1 = trắng, giống frame K1. Ghi bằng thư viện chuẩn Python, không cần Pillow. |
| PBM P4 | 1 = đen, ngược frame K1. |

Ảnh có cỡ 240 × 416 nhân `--scale`, đúng hướng frame dọc. Canvas dùng góc xoay `INK_UI_ROTATION_0`.

**Cách chạy bên trong**

1. `tools/render_view.py` đọc JSON, ghi `build-render/render_main.c` chứa struct dữ liệu dạng designated initializer.
2. CMake ở `tools/render_view/` build `render_main.c`, file `.gen.c` của view và các khối C thuần (giống `test/host`).
3. Chương trình vẽ vào frame trắng, ghi frame thô ra file. Python đổi frame thành PNG hoặc PBM.

`tools/build_all.py` chạy renderer với view mẫu `example_view` sau bước host test, để renderer không bị hỏng âm thầm.

### View mẫu trong template app

Template app `templates/components/ink_app_example/views/` có view `example_view` viết tay theo đúng dạng công cụ sinh
view sẽ ghi. View này có đủ ba loại field (`{title}`, `*rows` với `{label}`, `?empty_hint`) và hai part (`header`,
`body`). Scene của template gọi `example_view_draw()` khi vào màn và `example_view_draw_body()` khi đổi số hàng.

`tools/new.py app <tên>` đổi `example_view` thành `<tên>_view` cả trong tên file lẫn nội dung, để hai app tạo từ
template không trùng tên hàm.

### Luồng làm việc

1. Người thiết kế vẽ màn trong Figma theo design system của panel: frame 240 × 416, text style `unifont_vi_16` hoặc
   `fixed_10`, màu đen, trắng, xám.
2. Công cụ sinh view đọc màn qua Figma MCP, kiểm theo bảng "Luật kiểm", ghi `.gen.c`, `.gen.h` và `.sample.json`.
3. `python tools/render_view.py <view>` vẽ view ra PNG, so với ảnh xuất từ Figma.
4. Build firmware, nạp board, chụp màn bằng `tools/vision_test`.
