# Cấu trúc code

## Vấn đề

Code do cả người làm và AI viết. Không có khung chung thì mỗi lần viết một kiểu: tên lộn xộn, khối gọi chéo nhau,
logic dính FreeRTOS nên không test được trên PC, và code dần lệch khỏi thiết kế mà không ai biết.

## Cách xử lý

### Repo

```text
CMakeLists.txt         project firmware
cmake/ink.cmake        hàm CMake chung: ink_core_component, cờ build
main/                  app_main: chỉ lắp các khối với nhau, không có logic
components/
  core/                C thuần, không include gì của ESP-IDF
    ink_session/ ink_gesture/ ink_keymap/ ink_scene/ ink_study/ ink_fsrs/ ink_record/ ink_ui/ ink_display_policy/
  ink_system/ ink_display/ ink_storage/ ink_platform/
  ink_hal/             interface phần cứng
  ink_bsp/             bảng chân theo board
  drivers/ink_drv_<chip>/
  apps/ink_app_<tên>/
templates/             project ESP-IDF riêng chứa khối mẫu, build được
test/
  host/                project CMake build khối C thuần bằng gcc trên PC
  vectors/             test vector dùng chung mọi nền tảng
tools/                 new.py, check.py, build_all.py
```

Tên thư mục của component chính là tên component (luật của ESP-IDF), nên thư mục cũng mang tiền tố `ink_`.

### Đặt tên

| Loại | Quy tắc | Ví dụ |
|---|---|---|
| Component | Tiền tố `ink_` | `ink_display`, `ink_hal` |
| Hàm public | `ink_<khối>_<động từ>` | `ink_study_next_card` |
| Hàm, biến nội bộ | `static`, không tiền tố; biến bắt đầu `s_` | `choose_mode`, `s_state` |
| Hàm dùng chung giữa các file trong một component | `<khối>_<động từ>`, không có `ink_`; khai báo trong `<component>_internal.h` đặt cạnh file `.c`, không đặt trong `include/` nên khối khác không include được | `input_log_gestures` trong `ink_input_internal.h` |
| Kiểu dữ liệu | `ink_<khối>_<tên>_t` | `ink_gesture_event_t` |
| Mọi tên | Viết đủ từ, không viết tắt. Được giữ: đơn vị (`ms`, `mv`), chỉ số vòng lặp `i`, tên quen của C và ESP-IDF (`argc`, `argv`, `TAG`, `ISR`, `GPIO`, `u32`), tên chân và tín hiệu theo datasheet (`sda`, `scl`, `mosi`, `cs`, `bclk`) | `button`, `config`, `message`, không `btn`, `cfg`, `msg` |

Tiền tố `ink_` tránh trùng tên với component của ESP-IDF (IDF đã có `hal`, `esp_system`...).

Component dài thì tách file theo nơi code chạy, không theo kích thước. Ví dụ `ink_input`: `ink_input.c` giữ trạng thái
và chạy trên task main, `ink_input_console.c` chạy trên task REPL và chỉ đẩy message, `ink_input_log.c` là hàm thuần
không giữ trạng thái. Biến trạng thái vẫn `static` trong một file, file khác muốn đọc hay đổi thì gọi hàm.

### Luật phụ thuộc và cách ép

| Luật | Cách ép |
|---|---|
| Khối trong `core/` không dùng ESP-IDF | Host test biên dịch chúng bằng gcc trên PC; include header IDF là lỗi build |
| App không gọi `storage` hay `hal` | CMake của app không khai báo hai khối này; include là lỗi build |
| Chỉ `bsp` biết chân GPIO | Bảng chân nằm trong component `bsp`, không công khai |
| Bên giữ trạng thái của khối lõi không đụng trường bên trong | Trường nội bộ gom trong struct con `priv`; `tools/check.py` báo lỗi khi `.priv` hoặc `->priv` xuất hiện ngoài thư mục khối lõi |
| Định dạng code | `.clang-format` |

Mỗi luật viết một lần trong hàm CMake chung (ví dụ `ink_core_component(...)` tự chặn IDF và tự đưa khối vào host
test). CMake của từng khối chỉ gọi hàm đó, không chép lại luật.

### Các khối nói chuyện bằng gì

| Giữa | Kiểu | Ví dụ |
|---|---|---|
| Vỏ ↔ lõi C thuần | Sự kiện vào, hành động ra (struct) | `session` nhận "hết giờ chờ", trả "lưu snapshot", "ngủ" |
| `study` ↔ storage, đồng hồ | Cổng: struct con trỏ hàm, cắm lúc khởi tạo | Trên PC cắm file thử, đồng hồ giả |
| `display`, `system` ↔ driver | Bảng hàm (operations) + bảng khả năng (capabilities) | Driver màn điền `draw`, `load_old_image` |
| Ngắt, task phụ → task main | Hàng đợi của task main | Ngắt nút đẩy "nút B nhả lúc t" |

### Chuẩn C

Mọi khối viết theo C23. Trên máy, ESP-IDF v6.1 biên dịch bằng `gnu23` (mặc định). Host test biên dịch khối trong
`core/` bằng `-std=c23 -pedantic`, nên khối lõi không dùng phần mở rộng GNU.

App điện thoại và Kindle tự viết lại logic và chạy chung bộ test vector, không biên dịch lại code C của máy, nên
không cần giữ chuẩn C cũ hơn.

Nên dùng: `nullptr` thay `NULL`; `bool`, `true`, `false` không cần `<stdbool.h>`; `[[nodiscard]]` cho hàm trả mã lỗi;
enum có kiểu nền (`enum : uint8_t`) cho dữ liệu lưu xuống flash hay RTC memory.

### Mã lỗi

Khối trong `core/` không include `esp_err.h`. Mỗi khối lõi có enum lỗi riêng, chỉ gồm lỗi nó thật sự trả về.
Khối vỏ đổi sang `esp_err_t` khi trả lên trên.

### Template

`templates/` chứa khối mẫu nhỏ nhất mà vẫn build và chạy được, build cùng repo nên không bị cũ.

| Template | Dùng khi tạo | Nằm ở |
|---|---|---|
| `core` | Khối C thuần: header, `.c`, CMake, host test | `templates/components/ink_example/` |
| `shell` | Khối vỏ có task và hàng đợi | `templates/components/ink_example_shell/` |
| `driver` | Driver chip: bảng hàm, bảng khả năng, khởi tạo nhận chân qua tham số | `templates/components/ink_drv_example/` (làm theo interface mẫu `ink_hal_example`) |
| `host_test` | Test trên PC bằng Unity (lấy mã nguồn Unity có sẵn trong ESP-IDF) | `templates/components/ink_example/test/` |
| `app` | App 5 hàm, một scene, snapshot | Tạo cùng lúc với `ink_system`, vì app cần interface app của `ink_system` |

`templates/components/ink_periph_examples/` chứa mẫu dùng đúng API ESP-IDF v6.1 cho từng ngoại vi: mỗi ngoại vi một file.

`templates/` là một project ESP-IDF riêng, không nằm trong firmware. `tools/build_all.py` build cả firmware,
`templates/` và host test, nên template hỏng là biết ngay.

- Tạo khối mới: `tools/new.py <loại> <tên>` (loại: `core`, `shell`, `driver`). Script chép template và đổi tên theo quy tắc ở trên. AI luôn bắt đầu
  từ lệnh này, không tự viết khung.
- Docs chỉ ghi tên template, không chép code template vào docs.

### Mỗi file code trỏ về thiết kế

Dòng đầu mỗi file: `Thiết kế: docs/design/<file>.md`. Đổi một file thiết kế thì grep ra những file code cần xem lại.

### Script kiểm tra

`tools/check.py` chạy trước mỗi commit. Lệch thì báo lỗi kèm template đúng, và code phải viết lại cho khớp.

| Kiểm tra | |
|---|---|
| Tên hàm, tên file | Đúng quy tắc đặt tên |
| File bắt buộc | Khối có đủ file như template của nó (ví dụ khối lõi phải có host test) |
| `#if BOARD` | Chỉ nằm trong `bsp/` |
| Định dạng | Đúng `.clang-format` |
| Dòng `Thiết kế:` | Có, và file docs đó tồn tại |
| API cấm | Không dùng API ESP-IDF cũ hay API Arduino |
| `priv` | Truy cập `priv` chỉ nằm trong thư mục khối lõi |
