# App framework

## Vấn đề

Máy chạy nhiều app: Home, Settings và các app sau này. Mỗi app có nhiều màn. Deep sleep làm mất RAM, nên wake xong
máy phải về đúng app, đúng màn, đúng thẻ đang xem.

Ví dụ: đang ở màn Đáp án của thẻ #42 thì máy ngủ. Bấm nút, máy phải về lại màn Đáp án của thẻ #42. Bấm `BACK`, máy về
màn Mặt trước của thẻ #42.

## Cách xử lý

### Home

Máy boot vào Home. Home là app như mọi app: liệt kê các app trong danh sách app và mục Ngủ.

| Tình huống | Kết quả |
|---|---|
| `HOLD_AB_SHORT` ở bất kỳ app nào | `system` đóng app đang mở, mở Home |
| `BACK` ở scene gốc của app khác Home | `system` đóng app, mở Home |
| `BACK` ở scene gốc của Home | Bỏ qua |

Settings là một app. Settings đọc và ghi cấu hình thiết bị qua API của `system`, không đụng thẳng NVS.

### App gồm hàm vòng đời, app table và scene

| Phần | Việc |
|---|---|
| `start(snapshot)` | Mở app, cấp tài nguyên dùng chung cho các scene; `snapshot` rỗng nếu mở mới, có thì khôi phục state cấp app |
| `save(snapshot)` | Ghi state cấp app trước khi ngủ hoặc đóng app; app không có state cấp app thì để trống |
| `stop()` | Đóng app, giải phóng mọi thứ đã cấp ở `start` |
| App table | Danh sách command của app kèm gesture mặc định; Settings đọc bảng này để người dùng gán lại gesture |
| Danh sách scene | Mọi scene của app, mỗi scene một `scene_id`; scene đầu tiên là scene gốc |

App không có hàm nhận command hay hàm vẽ. Mọi command và mọi yêu cầu vẽ lại đi tới scene trên đỉnh scene stack.

App chạy trên main task. App chỉ gọi `system`, `study`, `display`, `ui`; không gọi thẳng `storage` hay phần cứng.

### Scene có 3 hàm

Mỗi màn của app là một scene. Mỗi tầng của scene stack gồm `scene_id` và 8 byte scene state.

| Hàm | Khi nào được gọi |
|---|---|
| `enter(state, restored)` | Scene vừa được push hoặc replace (`restored` = false), hoặc được dựng lại từ snapshot sau wake (`restored` = true) |
| `command(state, c)` | Có command cho scene trên đỉnh; trả về đã xử lý hay không |
| `draw(state)` | Cần vẽ lại cả màn: sau `enter`, sau wake, sau khi scene trên bị pop |

Ví dụ app Flashcard: scene stack là `[{FRONT, card=42}, {ANSWER, card=42}]`. Bấm `BACK` thì `ANSWER` bị pop, `system`
gọi `draw` của `FRONT` với state `card=42`.

- Scene ghi state thẳng vào 8 byte của tầng mình mỗi khi state đổi. State là số, không có pointer.
- Scene không cấp phát bộ nhớ. Tài nguyên dùng chung do app cấp ở `start`.
- Thay đổi nhỏ trong `command` (đổi mục chọn, lật thẻ) thì scene tự vẽ ngay, không chờ `draw`.

### Scene vẽ thế nào

Scene là người vẽ. `system` không đưa canvas cho scene.

1. Scene gọi `ink_display_acquire()`, bind canvas vào frame vừa acquire.
2. Scene vẽ bằng `ui`.
3. Scene submit với intent và flag nó chọn. Một thay đổi có thể submit hai frame: `UI` rồi `TEXT`.
4. Scene vẽ frame thứ hai sau khi `system` chuyển report của frame thứ nhất tới (report đi qua main queue).

### Scene stack

Scene stack tối đa 4 tầng. Module `scene` là core module, test trên PC.

| Thao tác | Dùng khi | Ví dụ |
|---|---|---|
| `push` | Mở màn con, `BACK` sẽ quay lại màn hiện tại | Danh sách thẻ → Mặt trước |
| `pop` | Đóng màn hiện tại | `BACK` |
| `replace` | Đổi màn hiện tại, độ sâu giữ nguyên | Mặt trước → Đáp án của cùng thẻ |
| `reset` | Về scene gốc, bỏ mọi tầng trên | Xong phiên ôn, về danh sách |

`push` khi đã đủ 4 tầng thì trả lỗi; scene stack giữ nguyên, scene gọi `push` tự xử lý lỗi.

### Đường đi của command

1. `input` gọi `gesture`, ra gesture.
2. `HOLD_AB_SHORT` và `HOLD_AB_LONG`: `system` xử lý luôn, app không thấy.
3. Gesture khác: `system` tra `ink_keymap_lookup()` với scene table của scene trên đỉnh và app table của app đang mở.
   Không tra ra command thì gesture bị drop.
4. `system` gọi `command` của scene trên đỉnh.
5. Scene báo không xử lý `BACK`: `system` pop scene stack rồi gọi `draw` của scene mới trên đỉnh. Scene vừa pop là
   scene gốc thì `system` về Home.

Mỗi lần mở app hoặc đổi scene, `system` tính lại wait mask của từng nút bằng `ink_keymap_wait_mask()`.

### Snapshot của app

Snapshot của app gồm scene stack (mỗi tầng `scene_id` và 8 byte scene state) và state cấp app do `save` ghi. Snapshot
nằm trong RTC slot của app. Một cơ chế lo cả `BACK` lẫn wake: wake xong, `system` dựng lại scene stack, gọi `enter` với
`restored` = true cho từng tầng từ dưới lên, rồi gọi `draw` của scene trên đỉnh.

### Việc của system và của app

| Việc | Ai lo |
|---|---|
| Background frame khi ngủ (số thẻ cần ôn, pin) | `system`, lấy số thẻ từ `study` |
| `HOLD_AB_SHORT` (về Home), `HOLD_AB_LONG` (tắt nguồn) | `system` nhận trước, app không thấy |
| Pop scene stack khi scene không xử lý `BACK`; về Home khi pop scene gốc | `system` |
| Giữ máy thức (ví dụ đang phát âm thanh) | App xin `system`, tối đa 30 s mỗi lần xin |

### Danh sách app

Danh sách app cố định, biên dịch sẵn vào firmware. App chỉ gọi qua API của các module, không dùng biến của module khác,
để sau này nạp được app lúc chạy (như file `.fap` của Flipper Zero).

### Rò rỉ bộ nhớ

Ở bản build debug, `system` đo heap trước và sau mỗi lần chạy app; lệch thì ghi log.
