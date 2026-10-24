# Cấu trúc code

## Vấn đề

Code do cả người làm và AI viết. Không có khung chung thì mỗi lần viết một kiểu: tên lộn xộn, khối gọi chéo nhau,
logic dính FreeRTOS nên không test được trên PC, và code dần lệch khỏi thiết kế mà không ai biết.

## Cách xử lý

### Repo

```text
CMakeLists.txt
main/                  app_main: chỉ lắp các khối với nhau, không có logic
components/
  core/                C thuần, không include gì của ESP-IDF
    session/ gesture/ scene/ study/ fsrs/ record/ ui/
  system/ display/ storage/ platform/
  hal/                 interface phần cứng
  bsp/                 bảng chân theo board
  drivers/<chip>/
  apps/menu/ apps/flashcard/ apps/settings/
templates/             khối mẫu, build được
test/
  host/                test C thuần, build bằng gcc trên PC
  vectors/             test vector dùng chung mọi nền tảng
tools/                 new.py, check.py, script build mọi board
```

### Đặt tên

| Loại | Quy tắc | Ví dụ |
|---|---|---|
| Component | Tiền tố `ink_` | `ink_display`, `ink_hal` |
| Hàm public | `ink_<khối>_<động từ>` | `ink_study_next_card` |
| Hàm, biến nội bộ | `static`, không tiền tố; biến bắt đầu `s_` | `choose_mode`, `s_state` |
| Kiểu dữ liệu | `ink_<khối>_<tên>_t` | `ink_gesture_event_t` |

Tiền tố `ink_` tránh trùng tên với component của ESP-IDF (IDF đã có `hal`, `esp_system`...).

### Luật phụ thuộc và cách ép

| Luật | Cách ép |
|---|---|
| Khối trong `core/` không dùng ESP-IDF | Host test biên dịch chúng bằng gcc trên PC; include header IDF là lỗi build |
| App không gọi `storage` hay `hal` | CMake của app không khai báo hai khối này; include là lỗi build |
| Chỉ `bsp` biết chân GPIO | Bảng chân nằm trong component `bsp`, không công khai |
| Định dạng code | `.clang-format` |

Mỗi luật viết một lần trong hàm CMake chung (ví dụ `ink_core_component(...)` tự chặn IDF và tự đưa khối vào host
test). CMake của từng khối chỉ gọi hàm đó, không chép lại luật.

### Các khối nói chuyện bằng gì

| Giữa | Kiểu | Ví dụ |
|---|---|---|
| Vỏ ↔ lõi C thuần | Sự kiện vào, hành động ra (struct) | `session` nhận "hết giờ chờ", trả "lưu snapshot", "ngủ" |
| `study` ↔ storage, đồng hồ | Cổng: struct con trỏ hàm, cắm lúc khởi tạo | Trên PC cắm file thử, đồng hồ giả |
| `display`, `system` ↔ driver | Bảng hàm (ops) + bảng khả năng | Driver màn điền `draw`, `load_old_image` |
| Ngắt, task phụ → task main | Hàng đợi của task main | Ngắt nút đẩy "nút B nhả lúc t" |

### Mã lỗi

Khối trong `core/` không include `esp_err.h`. Mỗi khối lõi có enum lỗi riêng, chỉ gồm lỗi nó thật sự trả về.
Khối vỏ đổi sang `esp_err_t` khi trả lên trên.

### Template

`templates/` chứa khối mẫu nhỏ nhất mà vẫn build và chạy được, build cùng repo nên không bị cũ.

| Template | Dùng khi tạo |
|---|---|
| `core_module` | Khối C thuần: header, `.c`, CMake, host test |
| `shell_module` | Khối vỏ (có hoặc không có task) |
| `driver` | Driver chip: bảng hàm, bảng khả năng, khởi tạo nhận chân qua tham số |
| `app` | App 5 hàm, một scene, snapshot |
| `host_test` | Test trên PC, nạp test vector |

Template cũng là mẫu dùng đúng API ESP-IDF v6.1 cho từng ngoại vi.

- Tạo khối mới: `tools/new.py <loại> <tên>`. Script chép template và đổi tên theo quy tắc ở trên. AI luôn bắt đầu
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
