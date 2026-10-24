# Kế hoạch chuẩn hoá docs và code (bản nháp, chờ duyệt)

File này tạm thời. Sau khi làm xong, nội dung được chuyển vào `QUY-UOC-TAI-LIEU.md`, `firmware/coding-style.md`
và `BUILD-TRACK.md`, rồi xoá file này.

## 1. Vấn đề cần giải quyết

Ví dụ thực tế trong repo ngày 2026-10-24:

- Lộ trình nằm ở cả `CLAUDE.md` và `docs/firmware/architecture.md`. Agent sửa một chỗ, chỗ kia lỗi thời.
- `architecture.md` mô tả FSRS, SUSPEND/RESUME như đã có, nhưng code chưa có.
- Diff chưa commit của `architecture.md` căn lại toàn bộ bảng markdown: phần lớn trong 61 dòng đổi chỉ là dấu cách, nội dung không đổi.
- Tiền tố hàm lẫn lộn: `ink_bus_post`, `render_submit`, `app_alloc`, `bsp_init`.
- Log viết tiếng Việt không dấu ("that bai", "ro ri"), khó đọc và khó tìm kiếm.

Nguyên nhân chung: chưa có luật viết ra, nên mỗi phiên agent tự chọn một kiểu.

## 2. Luật docs (sẽ thành `docs/QUY-UOC-TAI-LIEU.md`)

### 2.1. Mỗi thông tin chỉ có một nơi sở hữu

Nơi khác chỉ được tóm tắt một dòng và dẫn link về nơi sở hữu.

| Thông tin | Nơi sở hữu duy nhất |
|---|---|
| Chân GPIO | `docs/hardware/pinmap.md` (code `bsp/include/boards/*.h` phải khớp) |
| Thông số linh kiện | `docs/hardware/<linh-kiện>.md` |
| Quyết định và lý do | `docs/adr/NNNN-*.md` |
| Các tầng, luật phụ thuộc | `docs/firmware/architecture.md` |
| Trạng thái, việc tiếp theo, nhật ký | `docs/BUILD-TRACK.md` |
| Chữ ký hàm, ý nghĩa mã lỗi | Comment trong header `.h`. Docs không chép chữ ký hàm. |
| Hợp đồng driver | `components/drivers/README.md` |
| Quy ước code | `docs/firmware/coding-style.md` |

### 2.2. Docs chỉ mô tả cái đã có trong code

Thứ chưa làm chỉ nằm trong `BUILD-TRACK.md`. Nếu cần nhắc trong file khác, ghi nhãn `(dự kiến GĐ<n>)`.

### 2.3. Luật cho agent khi viết docs

1. Không tạo file docs mới nếu chưa có mục trong `docs/README.md`. Tạo file mới thì thêm mục lục trong cùng lần sửa.
2. Tạo file mới thì chép từ `docs/_template/`, không tự nghĩ ra cấu trúc.
3. Chỉ sửa đoạn có nội dung đổi. Không căn lại bảng, không đổi xuống dòng ở đoạn không liên quan.
4. Giới hạn độ dài: file thường ≤ 150 dòng, ADR ≤ 40 dòng. Dài hơn thì tách file.
5. Không chép code vào docs. Dẫn tên hàm hoặc tên file.
6. Văn phong theo `C:\Users\PC\.claude\CLAUDE.md`: câu đủ ý, mỗi đoạn một ý, ví dụ trước lý thuyết sau.

### 2.4. Ký hiệu trạng thái (dùng trong `BUILD-TRACK.md`)

`[ ]` chưa làm · `[~]` đang làm · `[x]` xong · `[!]` docs và code lệch nhau · `—` không áp dụng (ghi lý do).

## 3. Luật code (sẽ thành `docs/firmware/coding-style.md`)

### 3.1. Đặt tên: tiền tố = tên header

Luật này máy kiểm được: hàm public trong `ink_render.h` phải bắt đầu bằng `ink_render_`.

| Loại | Quy tắc | Ví dụ |
|---|---|---|
| Header | `ink_<module>.h`; HAL là `ink_hal_<thiết bị>.h`; driver là `drv_<chip>.h` | `ink_render.h`, `ink_hal_display.h`, `drv_uc8253.h` |
| File `.c` | Trùng tên header | `ink_render.c` |
| Hàm public | `<tên header>_<động từ>` | `ink_render_submit`, `ink_hal_display_draw` |
| Hàm, biến `static` | Không tiền tố; biến bắt đầu `s_` | `choose_mode`, `s_lock` |
| Kiểu | `<tiền tố>_t` | `ink_render_hint_t` |
| Hằng, enum, macro | Chữ hoa, `INK_` | `INK_RENDER_FAST` |
| Kconfig | `CONFIG_INK_*` | `CONFIG_INK_BOARD_GD0` |

Đổi tên cần làm (khoảng 45 hàm):

| Hiện tại | Sau |
|---|---|
| `svc_render.h`: `render_*` | `ink_render.h`: `ink_render_*` |
| `svc_audio.h`: `audio_*` | `ink_audio.h`: `ink_audio_*` |
| `svc_storage.h`: `storage_*` | `ink_storage.h`: `ink_storage_*` |
| `ink_app.h`: `app_manager_*`, `app_alloc` | `ink_app_*` (`ink_app_start`, `ink_app_alloc`) |
| `bsp.h`, `bsp_pins.h`: `bsp_*` | `ink_bsp.h`, `ink_bsp_pins.h`: `ink_bsp_*` |
| `ink_hal_*.h`: `ink_display_*`, `ink_button_*`… | `ink_hal_display_*`, `ink_hal_button_*`… |
| `ink_review_log.h`: `ink_review_encode` | `ink_review_log_encode` |
| `ink_service.h`: `ink_services_start` | `ink_service_start_all` |
| `hal_*.c`, `svc_*.c`, `bsp.c` | Trùng tên header mới |

### 3.2. Ngôn ngữ trong code

- Tên biến, hàm, kiểu: tiếng Anh.
- Comment: tiếng Việt có dấu, vì người đọc chính là bạn.
- Log: tiếng Anh, ASCII, ngắn. Lý do: không vỡ font trên serial monitor, tìm kiếm dễ, đọc cùng log của IDF.
  Ví dụ: `"app_alloc %u B failed"` thay cho `"app_alloc %u B that bai"`.

### 3.3. Comment trong header

Mỗi hàm public có một comment ngay trên, nói đủ 4 điều (điều nào hiển nhiên thì bỏ):

1. Hàm làm gì.
2. Các mã lỗi trả về và khi nào.
3. Gọi được từ đâu: task nào, có gọi được từ ISR (hàm xử lý ngắt) không.
4. Ai sở hữu bộ nhớ truyền vào và trả ra.

### 3.4. Xử lý lỗi

- Hàm có thể lỗi trả về `esp_err_t`. Kết quả trả qua tham số `out_*`.
- Dùng `ESP_RETURN_ON_ERROR` / `ESP_GOTO_ON_ERROR` (`esp_check.h`) để trả lỗi lên trên và dọn tài nguyên.
- `ESP_ERROR_CHECK` (lỗi là reset máy) chỉ dùng trong `main` lúc khởi động.
- Bỏ qua kết quả có chủ đích thì viết `(void)` trước lời gọi.
- Kiểm tra con trỏ `NULL` trước khi ghi (lỗi hiện có ở `ink_config_get_u32`).

### 3.5. ISR và đồng bộ

- ISR có `IRAM_ATTR`. Mọi hàm ISR gọi cũng phải nằm trong IRAM.
- Trong ISR: không log, không `malloc`, chỉ dùng hàm `*_from_isr`.
- Biến đếm dùng chung giữa task và ISR dùng `<stdatomic.h>`.
- Trạng thái chia sẻ giữa task được bảo vệ bằng mutex. Nhánh lỗi phải trả trạng thái về như cũ
  (lỗi hiện có ở `render_submit`: gửi lệnh thất bại nhưng cờ `s_has_pending` vẫn bật).

### 3.6. Định dạng: ép bằng `clang-format`

`clang-format` là công cụ tự định dạng code C theo một file cấu hình `.clang-format`: thụt lề, vị trí dấu `{`,
độ dài dòng tối đa. Bấm lưu trong VS Code (đã có clangd) là file tự được định dạng. Nhờ vậy cả bạn và agent
không phải nghĩ về định dạng, và diff chỉ còn thay đổi thật. Cấu hình đề xuất theo kiểu code hiện tại:
thụt 4 dấu cách, dòng tối đa 120 ký tự, `{` của hàm xuống dòng, `{` của `if`/`for` cùng dòng.

`.editorconfig` làm việc tương tự cho mọi loại file: UTF-8, xuống dòng LF, bỏ dấu cách thừa cuối dòng.

### 3.7. Test

- Mỗi hàm public của `domain/` và `ui/` có ít nhất một host test (test chạy trên PC bằng gcc).
- Sửa lỗi ở `domain/` hoặc `ui/` thì thêm test bắt được lỗi đó trước.

## 4. Cấu trúc sau khi chuẩn hoá

```text
CLAUDE.md                   ← ngắn: quyết định đã chốt (1 dòng + link), bảng "đổi X → cập nhật Y", checklist xong việc
README.md                   ← giới thiệu, lệnh build nhanh
.clang-format, .editorconfig
tools/check_repo.py         ← kiểm tra tự động (mục 5)
docs/
├── README.md               ← mục lục, mọi file docs phải có ở đây
├── QUY-UOC-TAI-LIEU.md
├── BUILD-TRACK.md          ← trạng thái, việc tiếp theo (≤ 5), nhật ký phiên
├── _template/              ← adr.md, hardware-part.md, component-readme.md
├── adr/
├── firmware/               ← architecture.md, coding-style.md, partitions.md
└── hardware/
components/<tên>/README.md  ← ≤ 30 dòng: mục đích, được phụ thuộc vào đâu, trạng thái
```

Bảng "đổi X → cập nhật Y" sẽ thêm vào `CLAUDE.md`:

| Khi đổi… | Phải cập nhật |
|---|---|
| Chân GPIO | `pinmap.md` và `bsp/include/boards/*.h` cùng lúc |
| Thêm hoặc đổi API public | Comment trong header; `architecture.md` nếu đổi luồng giữa các tầng |
| Thêm sự kiện bus | `ink_bus_evt_size()` và bảng sự kiện trong `architecture.md` |
| Quyết định mới hoặc đổi quyết định | ADR mới (không sửa ADR cũ, ghi "Thay bởi NNNN"), dòng tóm tắt trong `CLAUDE.md` |
| Xong một hạng mục | `BUILD-TRACK.md` mục 1 và mục 3 |
| Thêm file docs | Mục lục `docs/README.md` |

Checklist "trước khi báo xong": build qua, host test qua, `tools/check_repo.py` qua, đã rà bảng trên,
đã ghi nhật ký vào `BUILD-TRACK.md`.

## 5. Kiểm tra tự động: `tools/check_repo.py`

Luật chỉ viết ra thì agent vẫn quên. Script này biến luật thành lỗi, giống cách CMake đang ép luật phụ thuộc.

1. Link trong mọi file `.md` trỏ tới file có thật.
2. Mọi file trong `docs/` có trong mục lục `docs/README.md`.
3. Độ dài file docs không vượt giới hạn ở mục 2.3.
4. Hàm public trong header có tiền tố trùng tên header.
5. Chuỗi log chỉ có ký tự ASCII; không gọi thẳng `ESP_LOGx` (phải qua `INK_LOGx`).

## 6. Các bước làm

| Bước | Việc | Thời gian | Commit |
|---|---|---|---|
| 1 | Bạn duyệt file này | 10 phút | — |
| 2 | `.clang-format`, `.editorconfig`, sửa `.gitignore` (comment cuối dòng không có tác dụng) | 20 phút | 1 |
| 3 | Docs: `QUY-UOC`, `BUILD-TRACK`, `_template/`, `coding-style.md`; gọn lại `CLAUDE.md` và `architecture.md` | 1,5 giờ | 1 |
| 4 | Đổi tên header, hàm, file theo bảng 3.1; log sang tiếng Anh; build lại | 1–1,5 giờ | 1 |
| 5 | Sửa 3 lỗi đã tìm thấy (treo render, ISR không nằm trong IRAM, `NULL` ở config) | 20 phút | 1 |
| 6 | `tools/check_repo.py`, sửa hết lỗi nó báo | 45 phút | 1 |
| 7 | Chạy `clang-format` toàn bộ code | 10 phút | 1 riêng |

Tổng khoảng 4–5 giờ. Thư mục `components/drivers/` chỉ nhận luật đặt tên, không viết code driver thay bạn.
