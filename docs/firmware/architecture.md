# Kiến trúc firmware InkFlash 

Tài liệu này mô tả cách firmware được chia tầng, các tầng nói chuyện với nhau ra sao, và luật nào giữ cho kiến trúc

không bị phá. Lý do của từng quyết định lớn nằm trong `docs/adr/`.

## Ví dụ: bấm nút B để lật thẻ

1. **Driver nút** (`drivers/`, bạn viết) nhận ngắt GPIO và gọi `ink_button_report_from_isr(1, true)`.
2. **HAL** (`ink_hal/`) đóng dấu thời gian rồi phát sự kiện `INK_EVT_BUTTON_RAW` lên event bus.
3. **Service input** (`services/svc_input.c`) đo thời gian giữ nút và tra bảng ánh xạ: nút 1, nhấn đơn → `INK_INTENT_SELECT`. Nó phát `INK_EVT_INTENT`.
4. **App manager** (`app_fw/`) nhận intent và gọi `on_event(INK_INTENT_SELECT)` của app đang mở (Flashcard).
5. **App Flashcard** (`apps/`) chuyển scene Question → Answer, rồi gọi `app_request_redraw()`.
6. **App manager** gọi `on_draw(canvas)` để app vẽ mặt đáp án vào canvas, rồi gửi canvas cho `render_submit()`.
7. **Service render** (`services/svc_render.c`) chép canvas, trả về ngay, rồi trong task riêng gọi HAL màn → driver UC8253 vẽ ra panel.

Không bước nào biết bước khác dùng chip gì. App không biết nút B là GPIO46. Driver màn không biết đang hiện thẻ gì.

## Các tầng

| Tầng         | Thư mục               | Vai trò                                                                        | Được phép thấy                    |
| ------------- | ----------------------- | ------------------------------------------------------------------------------- | -------------------------------------- |
| Apps          | `components/apps`     | App Flashcard, Settings; mỗi app chia thành scene                             | app_fw, API service, domain, ui, core  |
| App framework | `components/app_fw`   | Interface app, app manager (app runner, `app_alloc`, watchdog), scene manager | services, ui, core                     |
| Domain        | `components/domain`   | Card, Deck, FSRS, ReviewLog. **C thuần**, test trên PC                  | chỉ thư viện C chuẩn               |
| UI            | `components/ui`       | Canvas 1bpp, font (GĐ1). **C thuần**, test trên PC                     | chỉ thư viện C chuẩn               |
| Services      | `components/services` | input, render, storage, audio, power, transfer; mỗi service một task          | core, ui, domain; ink_hal (riêng tư) |
| Core          | `components/core`     | Event bus, log, health, config, heap                                            | ESP-IDF                                |
| HAL           | `components/ink_hal`  | Interface phần cứng + vòng đời thiết bị                                  | core                                   |
| Drivers       | `components/drivers`  | Mỗi chip một component. **Bạn tự viết**                              | ink_hal, ESP-IDF                       |
| BSP           | `components/bsp`      | Bảng chân theo board; nơi duy nhất chọn driver                             | ink_hal, drivers                       |
| Main          | `main`                | Thứ tự khởi động, console chẩn đoán                                     | tất cả                               |

## Luật phụ thuộc và cách chúng được ép

Phụ thuộc chỉ đi từ trên xuống. Vi phạm thì build lỗi, không dựa vào ý thức:

| Luật                                 | Cơ chế ép                                            | Đã thử phá (2026-10-23)                        |
| ------------------------------------- | ------------------------------------------------------- | -------------------------------------------------- |
| App không gọi HAL                   | `services` khai báo `ink_hal` ở `PRIV_REQUIRES` | App include `ink_hal_display.h` → lỗi build    |
| Service không biết board            | `services` không `REQUIRES` bsp/driver             | Service include `bsp.h` → lỗi build            |
| Chỉ BSP và driver thấy bảng chân | `bsp_pins.h` nằm trong component bsp                 | App framework include `bsp_pins.h` → lỗi build |
| Domain và UI là C thuần            | Host test biên dịch chúng bằng gcc trên PC         | Domain include FreeRTOS → lỗi build host test    |

ESP-IDF tự thêm các component chung (FreeRTOS, log...) vào mọi component, nên CMake **không** chặn được domain
include FreeRTOS. Rào chắn của domain/ui là host test, phải chạy nó trước khi commit.

## Sự kiện và lệnh

- **Thông báo** ("đã xảy ra X") đi qua event bus, bọc `esp_event` với một loop riêng. Mọi handler chạy tuần tự
  trong task của loop, nên handler chỉ chép sự kiện vào queue của mình. Xem [ADR 0002](../adr/0002-event-bus-esp-event.md).
- **Lệnh** ("hãy làm Y") gọi hàm API của service, ví dụ `render_submit()`. Hàm API gửi message vào queue của
  service rồi trả về ngay.
- **Intent** là sự kiện duy nhất app nhận. Đổi cách điều khiển (thêm nút, nhấn kép, cử chỉ IMU) chỉ sửa service
  input, không sửa app.

## Vòng đời app và task

ESP-IDF không có khái niệm app hay tiến trình. Thiết kế tự dựng, chi tiết ở [ADR 0004](../adr/0004-vong-doi-app-task.md):

- **Service** là task sống suốt từ lúc boot, nhận `SUSPEND/RESUME` khi chuyển trạng thái nguồn.
- **App không phải task.** Mọi app chạy trong task `app_runner`; app manager gọi `on_start → on_event/on_draw → on_stop`.
- **Bộ nhớ của app** cấp qua `app_alloc()`. Khối nào còn sau `on_stop` bị thu hồi và ghi log là rò rỉ.
- **App treo** thì Task Watchdog reset chip. GĐ3 thêm đếm crash và safe mode.

## Bộ nhớ

Cấp phát động được phép, có quy tắc ([ADR 0003](../adr/0003-cap-phat-dong.md)):

- Cấm trong ISR và trong vòng lặp nóng (mỗi frame render, mỗi khối audio): các chỗ này dùng buffer cấp sẵn.
- Buffer DMA nằm trong RAM nội; dữ liệu lớn nằm trong PSRAM.
- Lệnh console `heap` cho biết heap còn trống, mức thấp nhất, khối liền lớn nhất (nhỏ dần nghĩa là đang phân mảnh).

## Hiển thị

Service render đã có:

- Hai buffer cấp lúc khởi động: `pending` trong PSRAM nhận canvas từ app; `shown` trong RAM nội có DMA để driver đẩy ra SPI.
- **Coalescing:** đang vẽ mà app gửi nhiều canvas mới thì chỉ vẽ canvas mới nhất.
- Phát `INK_EVT_RENDER_DONE` kèm thời gian vẽ, để đo.

GĐ1 thêm: dirty rect, chính sách tự chọn partial/full/clean, prefetch mặt đáp án, font tiếng Việt.

## Thêm phần cứng

1. Viết driver theo hợp đồng trong [`components/drivers/README.md`](../../components/drivers/README.md).
2. Khai báo chân trong `components/bsp/include/boards/<board>.h` (sửa `docs/hardware/pinmap.md` trước).
3. Đăng ký driver trong `bsp_init()` (`components/bsp/bsp.c`) và thêm component driver vào `REQUIRES` của bsp.

Service và app không phải sửa gì.

## Kiểm tra

- **Host test** (domain, ui), chạy từ thư mục gốc:
  ```bash
  cmake -S test/host -B build-host -G Ninja -DCMAKE_C_COMPILER=gcc
  cmake --build build-host && ctest --test-dir build-host --output-on-failure
  ```

  `cmake` và `ninja` lấy từ `C:\Espressif\tools\cmake\4.0.3\bin` và `C:\Espressif\tools\ninja\1.12.1`.
- **Trên máy:** console `ink>` có `status`, `heap`, `tasks`, `log`, và `intent <tên>` để bơm intent giả, chạy thử luồng app khi chưa có driver nút.

## Lộ trình

| GĐ | Nội dung                                                                                                 | Trạng thái                |
| --- | --------------------------------------------------------------------------------------------------------- | --------------------------- |
| 0   | Khung, core, HAL, host test, tài liệu                                                                   | **Xong** (2026-10-23) |
| 1   | Driver màn + nút (bạn viết); font tiếng Việt; render: dirty rect, chính sách làm mới, benchmark | Chưa                       |
| 2   | FSRS, deck, ReviewLog; storage trên LittleFS                                                             | Chưa                       |
| 3   | Input đầy đủ (kép, giữ, A+B); app Flashcard + Settings thật; safe mode                             | Chưa                       |
| 4   | Nạp deck từ PC (`tools/inkflash.py`)                                                                  | Chưa                       |
| 5   | Âm thanh: driver loa (bạn viết), service audio, ADPCM                                                  | Chưa                       |
| 6   | Năng lượng: deep sleep, thức về đúng thẻ, màn nền                                               | Chưa                       |
