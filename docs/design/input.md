# Nút và cử chỉ

## Vấn đề

Máy chỉ có 2 nút (và IMU), nên mỗi nút phải mang nhiều nghĩa: nhấn, giữ, tổ hợp A+B. App không được biết nút nào,
chỉ biết ý định (intent) như "lật thẻ", "chấm Good". Người dùng muốn tự gán cử chỉ nào làm chức năng nào. Quét nút
liên tục thì CPU không được ngủ, tốn pin.

## Cách xử lý

Ví dụ: app Flashcard gán mặc định "B nhấn" → `NEXT`. Người dùng vào Cài đặt đổi `NEXT` sang "B giữ". Từ đó giữ B
thì lật thẻ; code app không đổi.


| Lớp       | Đầu vào → đầu ra                                                                             | Khối     |
| ------------ | ---------------------------------------------------------------------------------------------------- | ----------- |
| Cử chỉ   | Nút bấm/nhả lúc t, tín hiệu IMU → cử chỉ (nút × kiểu: nhấn, nhấn kép, giữ, A+B...) | `gesture` |
| Bảng gán | Cử chỉ → intent, theo app và scene đang mở                                                   | `keymap`  |
| App        | Intent → hành động                                                                             | app       |

### Khối `gesture` là C thuần

Đầu vào: chuỗi "nút X bấm/nhả lúc t" và tín hiệu IMU. Đầu ra: cử chỉ. Không dính FreeRTOS hay phần cứng, nên mọi
trường hợp khó test được trên PC.

Mỗi cạnh nút vào kèm mức của chân (bấm hay nhả), không chỉ "có cạnh". Lọc rung làm trong `gesture`: nhận cạnh đầu
tiên ngay, bỏ các cạnh trong `debounce_ms` sau đó, hết khoảng này thì so mức cuối cùng với mức đã nhận.

### Cử chỉ gồm modifier và trigger

Cử chỉ là cặp (modifier, trigger).


| Trường        | Nghĩa                                         | Giá trị                                           |
| ----------------- | ------------------------------------------------ | ----------------------------------------------------- |
| trigger: nguồn | Cử chỉ đến từ đâu                       | Nút. IMU và giọng nói thêm vào trường này  |
| trigger: mã    | Nút nào                                      | A, B, A+B                                           |
| trigger: kiểu  | Làm gì                                       | Nhấn, nhấn kép, giữ, giữ A+B ngắn, giữ A+B dài |
| modifier        | Nút đang được giữ khi trigger bắt đầu | Không có, A, B                                    |

Ví dụ: giữ A rồi nhấn B ra (modifier A, trigger B nhấn).

Enum kiểu, mã, nguồn chỉ được thêm giá trị vào cuối, không đổi thứ tự: bảng gán người dùng sửa lưu cử chỉ dạng số,
đổi thứ tự thì bảng đã lưu trỏ sai cử chỉ mà không báo lỗi.

Mã nút và modifier là bitmask: mỗi nút một bit (A = 1, B = 2), tổ hợp là OR các bit (A+B = 3). Thêm nút thì thêm
bit, mọi tổ hợp tự có mã, mã cũ trong bảng gán đã lưu vẫn đúng. Host test giữ cố định giá trị số của các mã.

### Cạnh nào phát ra cử chỉ nào


| Cử chỉ                           | Phát lúc                                                                                                                                                                                                                        |
| ------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Nhấn, nút không bật nhấn kép | Nhả, nếu đã giữ dưới `hold_ms`                                                                                                                                                                                             |
| Nhấn, nút có bật nhấn kép    | Hết `double_ms` sau lần nhả mà không có lần bấm thứ hai                                                                                                                                                                  |
| Nhấn kép                         | Bấm lần thứ hai trong `double_ms`. Lần nhả sau đó bỏ qua                                                                                                                                                                  |
| Giữ, nút không bật modifier    | Giữ đủ `hold_ms`, không chờ nhả. Lần nhả sau đó bỏ qua                                                                                                                                                                 |
| Giữ, nút có bật modifier       | Nhả, nếu đã giữ đủ `hold_ms` và trong lúc giữ không bấm nút kia                                                                                                                                                      |
| A+B                                | Nút thứ hai bấm trong `combo_ms` sau nút đầu thì thành A+B. Phát lúc nhả nút cuối, nếu đã giữ dưới `ab_short_ms`                                                                                               |
| Giữ A+B ngắn                     | Nhả nút cuối, nếu đã giữ từ `ab_short_ms` tới dưới `ab_long_ms`. Phát lúc nhả để còn tới được mốc dài                                                                                                       |
| Giữ A+B dài                      | Giữ đủ `ab_long_ms`, không chờ nhả                                                                                                                                                                                          |
| Có modifier                       | Nút thứ hai bấm sau `combo_ms` khi nút đầu còn giữ và nút đầu có bật modifier. Modifier chốt lúc nút thứ hai bấm; cử chỉ của nút thứ hai theo các dòng trên. Nhả nút đầu sau đó không phát gì |

Nút thứ hai bấm sau `combo_ms` mà nút đầu không bật modifier thì bỏ qua cả hai nút cho tới khi nhả hết.

### Ngưỡng đổi được lúc chạy

Các ngưỡng thời gian (`debounce_ms`, `double_ms`, `hold_ms`, `combo_ms`, `ab_short_ms`, `ab_long_ms`) nằm trong một
struct cấu hình, đổi được lúc máy đang chạy, không cần build lại. `gesture` kiểm tra struct trước khi nhận, ví dụ
`ab_short_ms` phải nhỏ hơn `ab_long_ms`. `ab_long_ms` khoảng 30 s.

Lệnh console: `gesture show` in các ngưỡng, `gesture set <trường> <ms>` đổi một ngưỡng.

### Chạy theo sự kiện, không quét

- Ngắt GPIO khi chân nút đổi mức → callback của `system` đóng dấu thời gian bằng giờ của `platform` → sự kiện vào
  hàng đợi task main. Driver nút không đóng dấu thời gian, nên chỉ có một nguồn giờ.
- Hẹn giờ một lần của `platform` chỉ chạy khi đang có cử chỉ dở (ví dụ đang giữ nút, chờ xem có đủ `hold_ms` không).
  `gesture` cho biết thời điểm cần được gọi lại tiếp theo, hoặc không cần.
- Thời gian được truyền vào làm tham số, `gesture` không tự đọc đồng hồ.
- Callback hẹn giờ và lệnh console chạy trên task khác, nên chỉ đẩy message vào hàng đợi task main, không gọi
  thẳng `gesture`.


| Khối       | Việc                                                                                                            |
| ------------- | ------------------------------------------------------------------------------------------------------------------ |
| `bsp`       | Bảng chân nút: chân nào, kéo lên trong, mức khi bấm                                                     |
| Driver nút | Ngắt GPIO, đọc mức, gọi callback qua interface nút của `hal`                                              |
| `platform`  | Giờ, hẹn giờ một lần, console                                                                               |
| `system`    | Hàng đợi, đóng dấu thời gian, gọi `gesture`, hẹn giờ theo thời điểm `gesture` báo, lệnh `gesture` |
| `gesture`   | Lọc rung, máy trạng thái từng nút, ra cử chỉ                                                             |

### Ai nhận cử chỉ


| Cử chỉ            | Ai xử lý                                                                      |
| --------------------- | --------------------------------------------------------------------------------- |
| Giữ A+B ngắn      | Hệ thống: về menu. App không thấy, không gán lại được, nên người dùng luôn thoát được app. Đang ở menu thì bỏ qua |
| Giữ A+B dài       | Hệ thống: tắt nguồn (ship mode). App không thấy, không gán lại được   |
| Mọi cử chỉ khác | Tra bảng gán ra intent, đưa cho app đang mở. Cử chỉ chưa gán thì bỏ |

### Bảng gán (`keymap`)

- Mỗi app khai báo danh sách intent của mình: mã, tên hiện trong Cài đặt, cử chỉ mặc định.
- Intent chung (`NEXT`, `PREV`, `SELECT`, `BACK`) có mã cố định, dùng chung mọi app. Intent riêng của app đánh mã
  sau các mã chung.
- Bảng gán theo app. Scene nào cần nghĩa riêng thì app khai báo thêm bảng cho scene đó: tra bảng scene trước, không
  có thì tra bảng app.
- Khi lưu, `keymap` kiểm tra: mỗi cử chỉ chỉ trỏ tới một intent, và `BACK` luôn được gán, để người dùng không bị kẹt
  trong app.
- Cài đặt có mục khôi phục bảng gán mặc định.
- Phần tra và kiểm tra là C thuần, test trên PC. `system` giữ bảng của app đang mở và lưu bảng người dùng đã sửa.

### Lần bấm đánh thức

Lần bấm làm máy thức không tính là thao tác: lần nhả đầu tiên mà không thấy lần nhấn tương ứng thì bỏ qua.

### Kiểu cử chỉ bắt phải chờ thì bật riêng từng nút

Nút nào có nhấn kép thì mỗi lần nhấn đơn phải chờ hết khoảng chờ nhấn kép (thường ~400 ms) mới biết là nhấn đơn.
Nút nào làm modifier thì "giữ" phải chờ tới lúc nhả mới biết người dùng giữ để làm modifier hay để giữ.

Vì vậy mỗi nút có một mask các kiểu bắt phải chờ (nhấn kép, modifier), mặc định tắt hết. `system` tính mask từ bảng
gán đang dùng (bảng scene và bảng app) mỗi khi mở app hoặc đổi scene: bật kiểu nào bảng gán có dùng trên nút đó.

Mask của một nút đổi khi nút đó đang có cử chỉ dở thì cử chỉ dở bị bỏ, và nút phải nhả hẳn mới nhận cử chỉ mới.
Lý do: cử chỉ được tra theo scene ở lúc phát, nên phát tiếp sẽ biến lần bấm trong scene cũ thành lệnh trong scene
mới. Mất một lần bấm ít hại hơn, vì người dùng thấy màn đã đổi và bấm lại.

### Báo cho người dùng khi đang giữ

Ví dụ: người dùng giữ A+B để tắt nguồn. Màn hiện thanh đếm ngược tới `ab_long_ms`, nên người dùng biết máy đang
làm gì và còn bao lâu; nhả ra trước thì không tắt.

`gesture` cho biết cử chỉ nào đang dở, bắt đầu lúc nào, ngưỡng kế tiếp lúc nào; hàm này chỉ đọc trạng thái, không
phát cử chỉ. Đang có cử chỉ dở thì `system` hẹn thêm giờ để vẽ lại tiến độ.

| Thao tác | Màn hiện |
|---|---|
| Giữ A+B, đủ `ab_short_ms` | Báo nhả ra thì về menu |
| Giữ A+B tới `ab_long_ms` | Đếm ngược tới lúc tắt nguồn |
| Nhập macro nhiều bước | Số bước đã nhập |
| Giữ một nút (`hold_ms`) | Không hiện: màn e-ink vẽ một lần mất khoảng 350 ms, xong thì cử chỉ đã phát |

### IMU

IMU chỉ dùng khi máy thức (gõ, úp mặt, xoay màn). Không dùng IMU để đánh thức máy.

Driver IMU khai báo nó có tự nhận cú gõ, độ nghiêng không. Có thì dùng ngắt của chip IMU, đi đường giống nút.
Không có thì `gesture` tự tính từ dữ liệu IMU.

## Tham khảo

OneButton (thư viện Arduino): mỗi nút một máy trạng thái INIT → DOWN → UP → COUNT → PRESS. Học cách tách đọc chân
khỏi logic (`tick(bool level)`). Không chép cách gọi `tick()` liên tục và tự đọc `millis()`.

Component `espressif/button` của ESP-IDF (https://docs.espressif.com/projects/esp-iot-solution/en/latest/input_device/button.html):
học cách viết code: tạo nút theo từng loại thiết bị (`iot_button_new_gpio_device`, `..._adc_device`) qua bảng hàm,
đổi ngưỡng lúc chạy (`iot_button_set_param`), danh sách event đầy đủ khi cần thêm kiểu cử chỉ, và lỗi đã sửa ở
v4.1.6: giữ nút lúc khởi động lại thì không nhận nút. Không dùng thẳng vì: không có A+B và modifier cho nút GPIO,
logic chạy trong `esp_timer` nên không test được trên PC, và quét theo chu kỳ trong lúc bấm.
