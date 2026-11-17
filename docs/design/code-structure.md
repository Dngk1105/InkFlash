# Cấu trúc code

## Vấn đề

Code do cả người làm và AI viết. Không có cấu trúc chung thì mỗi lần viết một kiểu: tên lộn xộn, module gọi chéo nhau,
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
  ink_u8g2/            component bọc thư viện U8g2
  ink_demo/            vẽ thử ink_ui trên board (tạm)
  ink_hal/             interface phần cứng
  ink_bsp/             pin map theo board
  drivers/ink_drv_<chip>/
  apps/ink_app_<tên>/
third_party/           mã nguồn ngoài, mỗi thư viện một git submodule; tools/check.py không quét
  u8g2/
templates/             project ESP-IDF riêng chứa khối mẫu, build được
test/
  host/                project CMake build khối C thuần bằng gcc trên PC
  vectors/             test vector dùng chung mọi nền tảng
tools/                 new.py, check.py, build_all.py, profile_gen.py (sinh danh sách profile waveform),
                       u8g2_font.py (chép một font của U8g2 ra file .c)
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
| Header chỉ có kiểu dùng chung giữa nhiều tầng | `<tiền tố>_types.h` trong core module thấp nhất dùng nó; tiền tố là phần đầu tên component, mọi tên bên trong theo tiền tố đó. Không có hàm, không include ESP-IDF | `ink_display_types.h` trong `ink_display_policy` (`ink_display_cleanup_t` dùng chung cho policy, display, HAL); `ink_gesture_types.h` trong `ink_gesture` (`ink_gesture_t`, `ink_gesture_type_t`, `ink_gesture_config_t` dùng chung cho gesture, keymap, input) |
| Mọi tên | Viết đủ từ, không viết tắt. Được giữ: đơn vị (`ms`, `mv`), chỉ số vòng lặp `i`, tên quen của C và ESP-IDF (`argc`, `argv`, `TAG`, `ISR`, `GPIO`, `u32`), tên chân và tín hiệu theo datasheet (`sda`, `scl`, `mosi`, `cs`, `bclk`) | `button`, `config`, `message`, không `btn`, `cfg`, `msg` |

Tiền tố `ink_` tránh trùng tên với component của ESP-IDF (IDF đã có `hal`, `esp_system`...).

Component dài thì tách file theo nơi code chạy, không theo kích thước. Ví dụ `ink_input`: `ink_input.c` giữ state
và chạy trên main task, `ink_input_log.c` là hàm thuần không giữ trạng thái. Biến trạng thái vẫn `static` trong một file, file khác muốn đọc hay đổi thì gọi hàm.

### Viết code dễ đọc

Ví dụ: `refresh` trong `ink_drv_uc8253.c` đọc như danh sách bước: vào window, chọn nhiệt độ, bật nguồn, quét,
đổi vai hai vùng RAM. Mỗi bước là một lời gọi hàm có tên; muốn xem chi tiết thì mở hàm đó.

| Luật | Áp dụng |
|---|---|
| Hàm cấp trên đọc như danh sách bước; mỗi hàm khoảng 30 dòng trở xuống, không cứng | Code mới và `ink_display`; code cũ sửa khi đụng tới |
| State và quyết định nằm trong core module có host test; shell module chỉ giữ lock, task, queue | Mọi module |
| Điều kiện dài thì đặt tên bằng biến `bool` (ví dụ `other_fresh`, `held` trong `ink_gesture.c`) | Mọi khối |
| Driver không viết byte lệnh trần trong luồng chính: mỗi chuỗi lệnh gửi controller là một hàm hoặc hằng có tên theo datasheet, comment ghi mã lệnh | Driver |
| `if (...) { return ...; }` một dòng dùng cho điều kiện thoát sớm ở đầu hàm; giữa hàm viết khối nhiều dòng | Hướng dẫn, không kiểm |

`.clang-format` căn thẳng dấu `=` của các dòng liền nhau, nên có hai chỗ cần viết khác để khỏi bị thụt lề sâu:

- Khối khởi tạo nhiều dòng (`x = {` hay `x = (type){`) đặt sau một dòng trống, để `=` của nó không căn theo dòng trên.
- Struct lồng nhau: khởi tạo struct con thành biến riêng trước, rồi gán vào struct ngoài
  (ví dụ `capabilities` trong `ink_drv_uc8253_init`).

### Luật phụ thuộc và cách ép

| Luật | Cách ép |
|---|---|
| Khối trong `core/` không dùng ESP-IDF | Host test biên dịch chúng bằng gcc trên PC; include header IDF là lỗi build |
| Mã nguồn ngoài chỉ vào firmware qua một component bọc | Component bọc gọi `ink_third_party_component(...)`: liệt kê từng file được biên dịch, không áp `-pedantic` cho mã ngoài; khối khác include header `ink_<tên>.h` của component bọc |
| App không gọi `storage` hay `hal` | CMake của app không khai báo hai khối này; include là lỗi build |
| Chỉ `bsp` biết chân GPIO | Pin map nằm trong component `bsp`, không công khai |
| Caller giữ struct state của core module nhưng không đụng field bên trong | Field nội bộ gom trong struct con `priv`; `tools/check.py` báo lỗi khi `.priv` hoặc `->priv` xuất hiện ngoài thư mục của core module |
| Định dạng code | `.clang-format` |

Mỗi luật viết một lần trong hàm CMake chung (ví dụ `ink_core_component(...)` tự chặn IDF và tự đưa khối vào host
test). CMake của từng khối chỉ gọi hàm đó, không chép lại luật.

### Các khối nói chuyện bằng gì

| Giữa | Kiểu | Ví dụ |
|---|---|---|
| Shell module ↔ core module | Event vào, action ra (struct) | `session` nhận "hết giờ chờ", trả "lưu snapshot", "ngủ" |
| `study` ↔ storage, đồng hồ | Port: struct con trỏ hàm, cắm lúc init | Trên PC cắm file thử, đồng hồ giả |
| `display`, `input` ↔ driver | Operations + capabilities | Driver màn điền `draw`, `power_off`, `read_temperature` |
| ISR, helper task → main task | Main queue | ISR của nút post edge "nút B nhả lúc t" |

### Chuẩn C

Mọi khối viết theo C23. Trên máy, ESP-IDF v6.1 biên dịch bằng `gnu23` (mặc định). Host test biên dịch khối trong
`core/` bằng `-std=c23 -pedantic`, nên core module không dùng phần mở rộng GNU.

App điện thoại và Kindle tự viết lại logic và chạy chung bộ test vector, không biên dịch lại code C của máy, nên
không cần giữ chuẩn C cũ hơn.

Nên dùng: `nullptr` thay `NULL`; `bool`, `true`, `false` không cần `<stdbool.h>`; `[[nodiscard]]` cho hàm trả mã lỗi;
enum có kiểu nền (`enum : uint8_t`) cho dữ liệu lưu xuống flash hay RTC memory.

### Mã lỗi

Core module không include `esp_err.h`. Mỗi core module có enum lỗi riêng, chỉ gồm lỗi nó thật sự trả về.
Shell module đổi sang `esp_err_t` khi trả lên trên.

### Template

`templates/` chứa khối mẫu nhỏ nhất mà vẫn build và chạy được, build cùng repo nên không bị cũ.

| Template | Dùng khi tạo | Nằm ở |
|---|---|---|
| `core` | Khối C thuần: header, `.c`, CMake, host test | `templates/components/ink_example/` |
| `shell` | Shell module có task và queue | `templates/components/ink_example_shell/` |
| `driver` | Driver chip: operations, capabilities, init nhận chân qua tham số | `templates/components/ink_drv_example/` (làm theo interface mẫu `ink_hal_example`) |
| `host_test` | Test trên PC bằng Unity (lấy mã nguồn Unity có sẵn trong ESP-IDF) | `templates/components/ink_example/test/` |
| `app` | App có `start`, `save`, `stop`, app table và một scene gốc | `templates/components/ink_app_example/` (dùng interface app của `ink_system`) |

`templates/components/ink_periph_examples/` chứa mẫu dùng đúng API ESP-IDF v6.1 cho từng ngoại vi: mỗi ngoại vi một file.

`templates/` là một project ESP-IDF riêng, không nằm trong firmware. `tools/build_all.py` build cả firmware,
`templates/` và host test, nên template hỏng là biết ngay.

- Tạo khối mới: `tools/new.py <loại> <tên>` (loại: `core`, `shell`, `driver`, `app`; `app` tạo `components/apps/ink_app_<tên>/`). Script chép template và đổi tên theo quy tắc ở trên. AI luôn bắt đầu
  từ lệnh này, không tự viết từ đầu.
- Docs chỉ ghi tên template, không chép code template vào docs.

### Mỗi file code trỏ về thiết kế

Dòng đầu mỗi file: `/* Design: docs/design/<file>.md */`. Đổi một file thiết kế thì grep ra những file code cần xem lại.

### Script kiểm tra

`tools/check.py` chạy trước mỗi commit. Lệch thì báo lỗi kèm template đúng, và code phải viết lại cho khớp.

| Kiểm tra | |
|---|---|
| Tên hàm, tên file | Đúng quy tắc đặt tên |
| File bắt buộc | Module có đủ file như template của nó (ví dụ core module phải có host test) |
| `#if BOARD` | Chỉ nằm trong `bsp/` |
| Định dạng | Đúng `.clang-format` |
| Dòng `Design:` | Có, và file docs đó tồn tại |
| API cấm | Không dùng API ESP-IDF cũ hay API Arduino |
| `priv` | Truy cập `priv` chỉ nằm trong thư mục của core module |
