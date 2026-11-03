# Công cụ test màn

## Vấn đề

Muốn chọn bảng sóng, so bóng mờ, kiểm thang dọn hay đo tốc độ lật thì phải gửi ảnh tuỳ ý lên màn, chạy chuỗi ảnh với
tốc độ chỉnh được, rồi chấm điểm bằng mắt và lưu lại để so. Firmware chính không nên chứa phần này: lệnh chỉnh tay,
thống kê và đường gửi ảnh làm code dài và rối, trong khi người dùng không bao giờ cần.

## Cách xử lý

Ví dụ: cần biết dọn viền theo cờ có xoá được viền quanh ô vừa lật không. Nạp firmware test, mở trang công cụ trên
Chrome, chọn đường pipeline, chọn bài "Dọn viền theo cờ", chạy. Ô lật 20 lần với ý định `UI`, rồi máy nghỉ; sau
`edge_ms` các gói DRAWN của lượt dọn tâm và dọn quanh hiện ra kèm số điểm có cờ và thời gian DRF. Chụp ảnh trước và
sau lượt dọn, chấm điểm viền 1–5; trang ghi một dòng gồm điểm, cài đặt, nhiệt độ và thời gian vào
`tools/display_bench/results/`.

### Hai phần, tách khỏi firmware chính

| Phần | Nằm ở | Việc |
|---|---|---|
| Firmware test | `tools/display_bench/firmware/` | Project ESP-IDF riêng, dùng lại component của firmware chính (`ink_bsp`, driver, `ink_display`, `ink_display_policy`, `ink_input`). Giữ 64 slot ảnh trong PSRAM, vẽ theo lệnh của PC, báo thời gian từng lần vẽ, báo cử chỉ nút |
| Trang web | `tools/display_bench/web/` | Chạy trên Chrome hoặc Edge, nói chuyện với máy qua Web Serial (API mở cổng COM từ trang web). Xử lý ảnh, bài test, chuỗi ảnh, chấm điểm |
| Script phục vụ | `tools/display_bench/bench_server.py` | Thư viện chuẩn của Python. Phục vụ trang ở `http://localhost:8253` (Web Serial chỉ chạy trên `localhost`), ghi kết quả chấm điểm, liệt kê ảnh mẫu trong `images/` |

Firmware test nạp đè vào cùng phân vùng app với firmware chính; test xong thì nạp lại firmware chính. `tools/build_all.py`
build cả firmware test, nên component đổi API mà tool chưa sửa là build báo lỗi ngay.

Firmware chính chỉ có hai chỗ nối dùng chung với tool, và cả hai đều là API của sản phẩm: `ink_display_set_options` (app
Cài đặt cũng gọi) và callback `on_drawn` (luồng đi ngủ cũng chờ). Hai hàm chỉ tool gọi là nạp profile tạm và đọc OTP
(qua `ink_bsp`, gọi hàm riêng của driver UC8253), vì cả hai cần biết chip.

### Hai đường vẽ

| Đường | Vẽ thế nào | Dùng khi |
|---|---|---|
| raw | Task bench gọi thẳng `draw` của driver: profile, cấu hình vẽ, window (tuỳ ý, cả màn, hoặc vùng khung mới khác mặt phẳng cũ) và mặt phẳng cũ (ảnh đang hiện, một slot khác, khung mới đảo bit, hoặc bằng khung mới) đều chọn tay. Nguồn analog tự tắt sau 2 s không vẽ, chỉnh được bằng gói POWER_OFF | So bảng sóng, thử window, thử mặt phẳng cũ, đo nhiệt độ và dòng |
| pipeline | Chép slot vào khung mượn của `ink_display` rồi giao kèm ý định và cờ, như app thật: policy chọn preset, giữ nợ và cờ viền, chạy thang dọn khi nghỉ, tắt nguồn | Kiểm hành vi thật, đo từ lúc bấm nút, kiểm thang dọn |

`ink_display` giữ driver suốt đời, nên hai đường không chạy cùng lúc. Đổi đường thì máy ghi lựa chọn vào RTC memory rồi
khởi động lại. Mặc định là raw.

Nút chạy ở cả hai đường, qua `ink_input` như máy thật: mỗi cử chỉ gửi lên PC; bấm A vẽ khung sau trong chuỗi, bấm B vẽ khung
trước. Thời điểm edge nút gần nhất là mốc đo `input`.

### Giao thức

Mọi gói có dạng `A5 5A` | loại (1 byte) | seq (1 byte) | độ dài payload (2 byte) | payload | CRC-16/CCITT-FALSE (2 byte)
tính trên loại, seq, độ dài và payload. Số nhiều byte là little-endian. Log của ESP-IDF đi thành gói LOG, nên không lẫn
vào dữ liệu; byte ngoài gói (log của bootloader) bị bỏ.

Gói PC gửi xuống, mỗi gói được trả ACK (mã `esp_err_t`, 4 byte) cùng seq:

| Loại | Tên | Payload |
|---|---|---|
| `01` | HELLO | trống; trả INFO thay cho ACK |
| `02` | SET_PATH | đường (0 raw, 1 pipeline); khác đường đang chạy thì máy khởi động lại sau ACK |
| `03` | UPLOAD | slot, 0, offset (2), dữ liệu ảnh (tối đa 4096) |
| `04` | DRAW | slot, index profile, nguồn mặt phẳng cũ (0 đang hiện, 1 slot, 2 đảo bit, 3 bằng khung mới), slot cũ, vào chế độ partial, `PT_SCAN`, kéo viền màn, kiểu window (0 theo x/y/rộng/cao, 1 cả màn, 2 vùng khung mới khác mặt phẳng cũ; mặt phẳng cũ bằng khung mới thì cả màn; vùng trống thì không vẽ), x, y, rộng, cao (2 byte mỗi số). Chỉ đường raw |
| `05` | SUBMIT | slot, ý định (0 `AUTO`, 1 `TEXT`, 2 `UI`, 3 `PAGE`), cờ (bit 0 hoãn dọn, bit 1 trước khi ngủ). Đường raw bỏ ý định và cờ, vẽ theo gói DRAW gần nhất |
| `06` | PLAY | ms giữa hai khung (2), số vòng (0 = mãi), ý định, cờ, số slot, các slot |
| `07` | STOP | trống |
| `08` | POWER_OFF | trống: tắt nguồn analog ngay; 4 byte: số ms không vẽ thì tự tắt (mặc định 2000, 0 = chỉ tắt bằng gói này). Chỉ đường raw |
| `09` | PROFILE_LOAD | gói profile `.inkw` do `tools/profile_gen.py` sinh. Sai mã nhận dạng, phiên bản, độ dài hoặc CRC thì trả lỗi, giữ profile tạm cũ. Profile tạm là index cuối trong INFO |
| `0A` | SET_OPTIONS | cài đặt của `ink_display`, cùng bố cục với khối cài đặt trong INFO. Chỉ đường pipeline |
| `0B` | READ_TEMP | trống; trả TEMP. Chỉ đường raw (đường pipeline có nhiệt độ trong DRAWN của lần dọn mạnh) |
| `0C` | OTP_DUMP | trống; trả các gói OTP rồi ACK. Chỉ đường raw |

Khối cài đặt (trong SET_OPTIONS và INFO): với mỗi mục đích theo thứ tự `TEXT`, `UI`, `PAGE`, dọn mạnh, dọn nhanh, dọn
tâm viền, dọn quanh viền, dọn nhẹ: số dải, 9 dải (°C có dấu 1 byte, id profile 2 byte), vào chế độ partial, `PT_SCAN`,
kéo viền màn, trọng số nợ. Sau đó: `auto_ui_percent`, `debt_soft`, `debt_page`, `debt_ceiling`, `debt_ceiling_max`,
3 byte 0, `edge_ms`, `light_ms`, `strong_ms`, `power_off_ms` (4 byte mỗi số).

Gói máy gửi lên:

| Loại | Tên | Payload |
|---|---|---|
| `80` | ACK | mã lỗi (4) |
| `81` | INFO | đường, rộng (2), cao (2), x_align, số slot, số profile (gồm profile tạm ở cuối), bản firmware (độ dài + chữ), rồi mỗi profile: id (2), thời gian một lần vẽ ms (2), nguồn (0–8 = TR0–TR8, 255 = vẽ tay), tên (độ dài + chữ); rồi khối cài đặt đang dùng |
| `82` | DRAWN | slot, việc đã làm, mục đích (255 ở đường raw), nhiệt độ, nhiệt độ hợp lệ, nợ lớn nhất, trần nợ, khung (1 = lần vẽ một khung đã giao, 0 = lượt dọn hoặc tắt nguồn), id profile (2), 0 (2), lỗi (4), x, y, rộng, cao (2 mỗi số), số điểm có cờ viền, input ms, queue ms, diff ms, spi µs, panel µs, after µs, bật nguồn µs (4 mỗi số) |
| `83` | GESTURE | modifier, nút, loại cử chỉ, 0, thời điểm edge ms (4) |
| `84` | TEMP | °C, 0, 0, 0, lỗi (4) |
| `85` | LOG | một dòng chữ |
| `86` | OTP | offset (2), dữ liệu đọc bằng lệnh `0xA2`; nhiều gói cho tới hết dải TR8 |

Việc đã làm trong DRAWN: 0 không vẽ, 1 vẽ theo preset của ý định (hoặc vẽ raw), 2 dọn mạnh, 3 dọn nhanh, 4 dọn tâm
viền, 5 dọn quanh viền, 6 dọn nhẹ, 7 tắt nguồn.

Chuỗi ảnh: khung sau được giao sau khi khung trước vẽ xong cộng thêm số ms của PLAY, nên không khung nào bị bỏ. Ở đường
pipeline, gói DRAWN của các lượt dọn không tính là "khung trước vẽ xong".

### Bài đo

Mỗi bài trả lời một câu hỏi mà thiết kế đang chờ số đo. Bài chạy trên trang, kết quả chấm điểm ghi vào results.

| Bài | Đường | Trả lời |
|---|---|---|
| Loại bóng mờ | raw | Ô lật 10 lần cùng chỗ bằng một profile: bóng là viền, xám trong vùng, hay cả màn xám dần |
| Giữ màu 0 V | raw | Ô cờ đảo 100 lần bằng `tr8` rồi bằng `tr8_kk0`: điểm đứng yên có xám dần không |
| Dọn viền theo cờ | pipeline | Lật ô với ý định `UI`, nghỉ: lượt dọn tâm và quanh viền có xoá viền không; số điểm có cờ so với viền thấy trên ảnh chụp; DRF dọn dưới 100 ms không |
| Chữ cho `TEXT` | raw | Trang chữ cuộn bằng profile chọn: profile nào cho chữ rõ nhất trong 0,3–0,4 s |
| Thang dọn | pipeline | Lật nhiều lần rồi nghỉ qua cả ba bậc: đúng thứ tự, đúng thời điểm, đúng nợ sau mỗi bậc |
| Thời gian policy | pipeline | Đổi cả màn liên tục: đoạn diff (policy, trạng thái từng điểm trong PSRAM, mặt phẳng cũ) mất bao lâu |
| `PT_SCAN` | raw | Vẽ ô A, rồi vẽ ô B ở chỗ khác với `PT_SCAN` 1 và 0: ô A có bị kéo theo không |
| Viền màn | raw | Vẽ cả màn trong và ngoài chế độ partial, có và không kéo viền: viền màn có đổi không |
| Nhiệt độ | raw | Đọc cảm biến nhiều lần giữa các lần vẽ (REG = 1): số có hợp lý so với nhiệt kế không |
| Giữ nguồn | raw | Vẽ, giữ nguồn bật trong N giây rồi tắt: đo dòng bằng đồng hồ ở hai trạng thái |
| Lật ô, Đảo cả màn, Quét cỡ window, Dải xám, Độ trễ nút | cả hai | Các bài cũ: lượt ghi RAM, độ đen trắng, thời gian theo cỡ window, dither, độ trễ từ lúc bấm |

### Kết quả chấm điểm

Mỗi lần chấm là một dòng JSON trong `tools/display_bench/results/<ngày>.jsonl`, gồm: điểm 1–5 (độ đen, độ trắng, bóng mờ,
viền, chớp, tổng thể), ghi chú, bài test, đường vẽ, profile (id, tên), cài đặt của đường pipeline, nhiệt độ, bản
firmware, và thời gian (trung bình, nhỏ nhất, lớn nhất của các gói DRAWN trong lần chạy, tách theo việc đã làm). File
này commit được, để so về sau.

### Chạy

1. Nạp firmware test trong terminal ESP-IDF: `idf.py -C tools/display_bench/firmware -p COM4 flash`.
2. Đóng `idf.py monitor` nếu đang mở: mỗi lúc chỉ một chương trình mở được cổng COM.
3. `python tools/display_bench/bench_server.py`, rồi mở `http://localhost:8253` trên Chrome hoặc Edge.
4. Test xong: `idf.py -p COM4 flash` để nạp lại firmware chính.
