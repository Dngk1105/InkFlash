# Nút và gesture

## Vấn đề

Máy chỉ có 2 nút (và IMU), nên mỗi nút phải mang nhiều nghĩa: bấm, giữ, bấm cả A và B. App không được biết nút nào;
app chỉ nhận command như "lật thẻ", "chấm Good". Người dùng muốn tự gán gesture nào làm chức năng nào. Quét nút
liên tục thì CPU không ngủ được, tốn pin.

## Cách xử lý

Ví dụ: app Flashcard gán mặc định "B `PRESS`" → `NEXT`. Người dùng vào app Settings đổi `NEXT` sang "B `HOLD`". Từ đó giữ B
thì lật thẻ; code app không đổi.

Sơ đồ cùng thư mục, vẽ từ code ở commit ghi trong mỗi file, mở bằng trình duyệt:

| File | Cho thấy |
|---|---|
| `architecture.html` | Các module firmware hiện có và ai gọi ai (chưa gồm display) |
| `sequence.html` | Một lần bấm nhanh nút A: ISR → main queue → main task → gesture, kể cả timer debounce và resync |
| `dataflow.html` | Dữ liệu đổi dạng qua từng tầng: level của chân → `pressed` → message → `ink_gesture_event_t` → `ink_gesture_t` |

Đổi luồng input trong code thì vẽ lại ba sơ đồ trong cùng commit.

| Tầng | Đầu vào → đầu ra | Module |
|---|---|---|
| Gesture | Edge của nút lúc t, tín hiệu IMU → gesture (button × type) | `gesture` |
| Keymap | Gesture → command, theo app và scene đang mở | `keymap` |
| App | Command → hành động | app |

### `gesture` là core module

Đầu vào: chuỗi edge "nút X bấm/nhả lúc t" và tín hiệu IMU. Đầu ra: gesture. `gesture` không dính FreeRTOS hay phần
cứng, nên mọi trường hợp khó đều test được trên PC. Kiểu dữ liệu dùng chung (`ink_gesture_t`, `ink_gesture_type_t`,
`ink_gesture_config_t`...) nằm riêng trong `ink_gesture_types.h`; `gesture` giữ state machine trong `ink_gesture.h`.

Mỗi edge mang level của chân sau edge (bấm hay nhả), không chỉ báo "có edge". Debounce làm trong `gesture`: nhận edge
đầu tiên ngay, bỏ mọi edge trong `debounce_ms` sau đó, hết khoảng này thì so level cuối với level đã nhận.

### Gesture gồm modifier và trigger

Gesture là cặp (modifier, trigger). Ví dụ: giữ A rồi bấm B ra (modifier A, trigger B `PRESS`).

| Field | Nghĩa | Giá trị |
|---|---|---|
| trigger: `source` | Gesture đến từ đâu | `BUTTON`. IMU và giọng nói thêm vào field này |
| trigger: `button` | Nút nào | A, B, A+B |
| trigger: `type` | Làm gì | `PRESS`, `DOUBLE_PRESS`, `HOLD`, `HOLD_AB_SHORT`, `HOLD_AB_LONG`, `HOLD_SHORT`, `HOLD_LONG` |
| `modifier` | Nút đang giữ khi trigger bắt đầu | `NONE`, A, B |

Enum `type`, `button`, `source` chỉ được thêm giá trị vào cuối, không đổi thứ tự. Keymap table người dùng sửa lưu
gesture dạng số; đổi thứ tự thì table đã lưu trỏ sai gesture mà không báo lỗi.

Mã `button` và `modifier` là bitmask: mỗi nút một bit (A = 1, B = 2), tổ hợp là OR các bit (A+B = 3). Thêm nút thì
thêm bit; mọi tổ hợp tự có mã, mã cũ trong keymap table đã lưu vẫn đúng. Host test giữ cố định giá trị số của các mã.

### Edge nào phát ra gesture nào

| Gesture | Phát lúc |
|---|---|
| `PRESS`, nút không bật `WAIT_DOUBLE` | Nhả, nếu đã giữ dưới `hold_short_ms` |
| `PRESS`, nút bật `WAIT_DOUBLE` | Hết `double_ms` sau lần nhả mà không có lần bấm thứ hai |
| `DOUBLE_PRESS` | Bấm lần thứ hai trong `double_ms`. Lần nhả sau đó không phát gì |
| `HOLD_SHORT` | Nhả, nếu đã giữ từ `hold_short_ms` tới dưới `hold_ms`. Phát lúc nhả để còn tới được `HOLD` và `HOLD_LONG` |
| `HOLD` | Nhả, nếu đã giữ từ `hold_ms` tới dưới `hold_long_ms`. Phát lúc nhả để còn tới được `HOLD_LONG` |
| `HOLD_LONG` | Giữ đủ `hold_long_ms`, không chờ nhả. Lần nhả sau đó không phát gì |
| `PRESS` của A+B | Nút thứ hai bấm trong `combo_ms` sau nút đầu thì thành A+B. Phát lúc nhả nút cuối, nếu đã giữ dưới `ab_short_ms` |
| `HOLD_AB_SHORT` | Nhả nút cuối, nếu đã giữ từ `ab_short_ms` tới dưới `ab_long_ms`. Phát lúc nhả để còn tới được `HOLD_AB_LONG` |
| `HOLD_AB_LONG` | Giữ đủ `ab_long_ms`, không chờ nhả |
| Có modifier | Nút thứ hai bấm sau `combo_ms`, khi nút đầu còn giữ và nút đầu bật `WAIT_MODIFIER`. Modifier chốt lúc nút thứ hai bấm; gesture của nút thứ hai theo các dòng trên. Nhả nút đầu sau đó không phát gì |

Nút thứ hai bấm sau `combo_ms` mà nút đầu không bật `WAIT_MODIFIER`: `gesture` bỏ qua cả hai nút cho tới khi nhả hết.

### Gesture config đổi được lúc chạy

Các ngưỡng thời gian (`debounce_ms`, `double_ms`, `hold_short_ms`, `hold_ms`, `hold_long_ms`, `combo_ms`, `ab_short_ms`, `ab_long_ms`) nằm trong
`ink_gesture_config_t`, đổi được lúc máy chạy, không cần build lại. `gesture` kiểm config trước khi nhận:
`debounce_ms < hold_short_ms < hold_ms < hold_long_ms`, `combo_ms < hold_short_ms`, và `ab_short_ms < ab_long_ms`.
Ngưỡng mặc định: `debounce_ms = 20 ms`, `double_ms = 400 ms`, `hold_short_ms = 600 ms`, `hold_ms = 2000 ms`, `hold_long_ms = 8000 ms`, `combo_ms = 100 ms`, `ab_short_ms = 2000 ms`, `ab_long_ms = 30000 ms`. Giá trị mặc định nằm trong `ink_input.c`.

App Settings đổi gesture config bằng `ink_input_set_gesture_config()`. `system` đặt wait mask của một nút bằng
`ink_input_set_wait_mask()` khi mở app hoặc đổi scene. Cả hai gọi trên main task.

### Chạy theo event, không quét

- Chân nút đổi level → GPIO interrupt → ISR của `input` lấy timestamp bằng `ink_platform_now_ms()` → post message vào
  main queue qua hàm post của `system`. Driver nút không lấy timestamp, nên chỉ có một nguồn giờ.
- Timer one-shot của `platform` chỉ chạy khi có gesture đang dở, ví dụ đang giữ nút, chờ xem đủ `hold_long_ms` chưa.
  `ink_gesture_next_deadline()` cho biết deadline tiếp theo, hoặc không cần timer.
- Thời gian truyền vào làm tham số; `gesture` không tự đọc đồng hồ.
- Timer callback chạy trên task `esp_timer` (một helper task), nên chỉ post message vào main queue, không gọi thẳng
  `gesture`.
- Main queue đầy thì message mới bị drop. Drop đúng edge cuối của một bounce thì `gesture` giữ level sai, ví dụ tưởng
  nút vẫn đang giữ, cho tới lần bấm sau. Vì vậy sau mỗi lần timer chạy hoặc có message bị drop, `system` báo cho `input`
  khi main queue rỗng, và `input` resync: đọc level thật của chân, khác level của edge cuối thì feed thêm một resync
  edge. Timestamp của các lần bấm, nhả vẫn lấy từ interrupt, nên không mất độ chính xác.
- Log progress chỉ in khi timer chạy, không in ở mỗi edge, để một bounce không in vài chục dòng.

| Module | Việc |
|---|---|
| `bsp` | Pin map của nút: chân nào, pull-up trong, level khi bấm. Cài GPIO ISR service một lần cho cả chip |
| `drv_gpio_button` | GPIO interrupt, đọc level, gọi callback qua interface `hal_button` |
| `platform` | Giờ, timer one-shot |
| `input` | Lấy timestamp, gọi `gesture`, đặt timer theo deadline, resync, nhận gesture config và wait mask mới. Không có task riêng: chạy trên main task khi `system` giao message |
| `system` | Main queue và main task: nhận message, giao cho module owner (`input`), nhận gesture ra |
| `gesture` | Debounce, state machine từng nút, ra gesture |

### Ai nhận gesture

| Gesture | Ai xử lý |
|---|---|
| `HOLD_AB_SHORT` | `system`: về menu. App không thấy, không gán lại được, nên người dùng luôn thoát được app. Đang ở menu thì bỏ qua |
| `HOLD_AB_LONG` | `system`: tắt nguồn (ship mode). App không thấy, không gán lại được |
| Mọi gesture khác | Tra keymap table ra command, đưa cho app đang mở. Gesture chưa gán thì bỏ |

### Keymap table (`keymap`)

- Mỗi app khai báo danh sách command của mình: mã, tên hiện trong app Settings, gesture mặc định.
- Command chung (`NEXT`, `PREV`, `SELECT`, `BACK`) có mã cố định 0 đến 3, dùng chung mọi app. Command
  riêng của app đánh mã từ 16 (`INK_KEYMAP_COMMAND_APP_BASE`); 4 đến 15 để dành cho command chung thêm sau. Mã lưu
  dạng số: command chung mới trùng mã riêng của app thì table đã lưu trỏ sai mà không báo lỗi.
- Một keymap table là danh sách entry `{gesture, command}`, tối đa 32 entry (`INK_KEYMAP_ENTRY_MAX`). Một
  command gán được nhiều gesture; một gesture chỉ trỏ tới một command.
- Keymap table theo app (app table). Scene cần nghĩa riêng thì app khai báo thêm scene table, chỉ gồm các entry khác
  app table. `keymap` tra scene table trước, không có thì tra app table.
- Khi lưu, `ink_keymap_validate()` kiểm effective table (scene table gộp với app table): không entry nào gán
  `HOLD_AB_SHORT` hay `HOLD_AB_LONG`, không gesture nào có hai entry trong cùng một table, và có ít nhất một gesture
  tra ra `BACK`, để người dùng không bị kẹt trong scene. Scene table đè mất `BACK` của app table thì bị từ chối.
- App Settings có mục khôi phục keymap table mặc định (`ink_keymap_reset()`).
- Tra và kiểm là core module, không giữ state, nhận con trỏ table (scene table có thể là `NULL`), test trên PC.
  `system` giữ app table và scene table đang mở, và lưu table người dùng đã sửa.

### Lần bấm đánh thức

Lần bấm làm máy wake không tính là thao tác: lần nhả đầu tiên mà không thấy lần bấm tương ứng thì bỏ qua.

### Wait mask bật riêng từng nút

Nút có `DOUBLE_PRESS` thì mỗi lần `PRESS` phải chờ hết `double_ms` (khoảng 400 ms) mới biết là `PRESS`. Nút làm modifier thì
phải chờ tới lúc nhả mới biết người dùng giữ để làm modifier hay để phát gesture.

Vì vậy mỗi nút có một wait mask (`WAIT_DOUBLE`, `WAIT_MODIFIER`), mặc định tắt hết. `system` tính wait mask bằng
`ink_keymap_wait_mask()` từ scene table và app table mỗi khi mở app hoặc đổi scene: bật bit nào keymap table có dùng
trên nút đó. Entry của app table bị scene table đè vẫn được tính, nên wait mask có thể bật dư, không bao giờ thiếu.

Wait mask của một nút đổi khi nút đó đang có gesture chưa phát: `gesture` huỷ gesture đó, và nút phải nhả hẳn mới nhận gesture
mới. Lý do: gesture được tra theo scene lúc phát, nên phát tiếp sẽ biến lần bấm trong scene cũ thành lệnh trong scene
mới. Mất một lần bấm ít hại hơn, vì người dùng thấy màn đã đổi và bấm lại.

### Báo progress khi đang giữ

Ví dụ: người dùng giữ A+B để tắt nguồn. Màn hiện thanh đếm ngược tới `ab_long_ms`, nên người dùng biết máy đang làm gì
và còn bao lâu; nhả ra trước thì không tắt.

`ink_gesture_progress()` cho biết gesture nào đang giữ dở (`on_release`), có ngưỡng kế tiếp không (`has_next`), type kế tiếp (`next`)
và lúc tới (`next_ms`); hàm chỉ đọc state, không phát gesture. Có progress thì `input` đặt timer thêm, mỗi 1 s một lần, để báo lại progress.

Với nút đơn:
- Dưới `hold_short_ms`: `on_release` là `PRESS`, `next` là `HOLD_SHORT` tại `hold_short_ms`.
- Từ `hold_short_ms` tới dưới `hold_ms`: `on_release` là `HOLD_SHORT`, `next` là `HOLD` tại `hold_ms`.
- Từ `hold_ms` tới dưới `hold_long_ms`: `on_release` là `HOLD`, `next` là `HOLD_LONG` tại `hold_long_ms`.
- Đủ `hold_long_ms`: `on_release` là `HOLD_LONG`, `has_next` là `false`.

| Thao tác | Màn hiện |
|---|---|
| Giữ nút đơn tới `hold_long_ms` (8 s) | Đếm ngược tới lúc phát `HOLD_LONG` |
| Giữ A+B đủ `ab_short_ms` | Báo nhả ra thì về menu (`HOLD_AB_SHORT`) |
| Giữ A+B tới `ab_long_ms` | Đếm ngược tới lúc tắt nguồn (`HOLD_AB_LONG`) |
| Nhập macro nhiều bước | Số bước đã nhập |
| Giữ nút đơn dưới `hold_short_ms` (600 ms) | Không hiện: panel vẽ một lần mất khoảng 350 ms, gần bằng ngưỡng |

### IMU

IMU chỉ dùng khi máy thức (gõ, úp mặt, xoay màn). Không dùng IMU để wake máy.

Capabilities của driver IMU khai báo nó có tự nhận cú gõ, độ nghiêng không. Có thì dùng interrupt của chip IMU, đi
đường giống nút. Không có thì `gesture` tự tính từ dữ liệu IMU.

## Tham khảo

OneButton (thư viện Arduino): mỗi nút một state machine INIT → DOWN → UP → COUNT → PRESS. Học cách tách đọc chân khỏi
logic (`tick(bool level)`). Không chép cách gọi `tick()` liên tục và tự đọc `millis()`.

Component `espressif/button` của ESP-IDF (https://docs.espressif.com/projects/esp-iot-solution/en/latest/input_device/button.html):
học cách viết code: tạo nút theo từng loại thiết bị (`iot_button_new_gpio_device`, `..._adc_device`) qua operations,
đổi ngưỡng lúc chạy (`iot_button_set_param`), danh sách event đầy đủ khi cần thêm gesture type, và lỗi đã sửa ở
v4.1.6: giữ nút lúc khởi động lại thì không nhận nút. Không dùng thẳng vì: không có A+B và modifier cho nút GPIO,
logic chạy trong `esp_timer` nên không test được trên PC, và quét theo chu kỳ trong lúc bấm.
