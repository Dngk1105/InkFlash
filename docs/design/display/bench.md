# Công cụ test màn

## Vấn đề

Muốn chọn waveform, so bóng mờ, kiểm cleanup stage hay đo tốc độ lật thì phải gửi ảnh tuỳ ý lên màn, chạy chuỗi ảnh với
tốc độ chỉnh được, rồi chấm điểm bằng mắt và lưu lại để so. Firmware chính không nên chứa phần này: lệnh chỉnh tay,
thống kê và đường gửi ảnh làm code dài và rối, trong khi người dùng không bao giờ cần.

## Cách xử lý

Ví dụ: cần biết cleanup theo edge flag có xoá được viền quanh ô vừa lật không. Nạp firmware test, mở trang công cụ trên
Chrome, chọn đường pipeline, chọn bài "Cleanup theo edge flag", chạy. Ô lật 20 lần với intent `UI`, rồi máy nghỉ; sau
`edge_ms` các gói DRAWN của cleanup edge center và edge neighbor hiện ra kèm số điểm có cờ và thời gian DRF. Chụp ảnh trước và
sau cleanup, chấm điểm viền 1–5; trang ghi một dòng gồm điểm, options, nhiệt độ và thời gian vào
`tools/display_bench/results/`.

### Hai phần, tách khỏi firmware chính

| Phần | Nằm ở | Việc |
|---|---|---|
| Firmware test | `tools/display_bench/firmware/` | Project ESP-IDF riêng, dùng lại component của firmware chính (`ink_bsp`, driver, `ink_display`, `ink_display_policy`, `ink_input`). Giữ 64 slot ảnh trong PSRAM, vẽ theo lệnh của PC, báo thời gian từng lần vẽ, báo gesture nút |
| Trang web | `tools/display_bench/web/` | Chạy trên Chrome hoặc Edge, nói chuyện với máy qua Web Serial (API mở cổng COM từ trang web). Xử lý ảnh, bài test, chuỗi ảnh, chấm điểm |
| Script phục vụ | `tools/display_bench/bench_server.py` | Thư viện chuẩn của Python. Phục vụ trang ở `http://localhost:8253` (Web Serial chỉ chạy trên `localhost`), ghi kết quả chấm điểm, liệt kê ảnh mẫu trong `images/` |

Firmware test nạp đè vào cùng phân vùng app với firmware chính; test xong thì nạp lại firmware chính. `tools/build_all.py`
build cả firmware test, nên component đổi API mà tool chưa sửa là build báo lỗi ngay.

Firmware chính chỉ có hai chỗ nối dùng chung với tool, và cả hai đều là API của sản phẩm: `ink_display_set_options` (app
Settings cũng gọi) và callback `on_report` (luồng đi ngủ cũng chờ). Hai hàm chỉ tool gọi là nạp profile tạm và đọc OTP
(qua `ink_bsp`, gọi hàm riêng của driver UC8253), vì cả hai cần biết chip.

### Hai đường vẽ

| Đường | Vẽ thế nào | Dùng khi |
|---|---|---|
| raw | Task bench gọi thẳng `draw` của driver: profile, cấu hình vẽ, window (tuỳ ý, cả màn, hoặc vùng frame mới khác old plane) và old plane (ảnh đang hiện, một slot khác, frame mới đảo bit, hoặc bằng frame mới) đều chọn tay. Analog power tự tắt sau 4 s không vẽ, chỉnh được bằng gói POWER_OFF | So waveform, thử window, thử old plane, đo nhiệt độ và dòng |
| pipeline | Chép slot vào frame acquire của `ink_display` rồi giao kèm intent và cờ, như app thật: policy chọn preset, giữ debt và edge flag, chạy cleanup stage khi idle, tắt nguồn | Kiểm hành vi thật, đo từ lúc bấm nút, kiểm cleanup stage |

`ink_display` giữ driver suốt đời, nên hai đường không chạy cùng lúc. Đổi đường thì máy ghi lựa chọn vào RTC memory rồi
khởi động lại. Mặc định là raw.

Nút chạy ở cả hai đường, qua `ink_input` như máy thật: mỗi gesture gửi lên PC; bấm A vẽ frame sau trong chuỗi, bấm B vẽ frame
trước. Thời điểm edge nút gần nhất là mốc đo `input`.

### Giao thức

Mọi gói có dạng `A5 5A` | loại (1 byte) | seq (1 byte) | độ dài payload (2 byte) | payload | CRC-16/CCITT-FALSE (2 byte)
tính trên loại, seq, độ dài và payload. Số nhiều byte là little-endian. Log của ESP-IDF đi thành gói LOG, nên không lẫn
vào dữ liệu; byte ngoài gói (log của bootloader) bị drop.

Gói PC gửi xuống, mỗi gói được trả ACK (mã `esp_err_t`, 4 byte) cùng seq:

| Loại | Tên | Payload |
|---|---|---|
| `01` | HELLO | trống; trả INFO thay cho ACK |
| `02` | SET_PATH | đường (0 raw, 1 pipeline); khác đường đang chạy thì máy khởi động lại sau ACK |
| `03` | UPLOAD | slot, 0, offset (2), dữ liệu ảnh (tối đa 4096) |
| `04` | DRAW | slot, index profile, nguồn old plane (0 đang hiện, 1 slot, 2 đảo bit, 3 bằng frame mới), slot cũ, vào chế độ partial, `PT_SCAN`, drive border, kiểu window (0 theo x/y/rộng/cao, 1 cả màn, 2 vùng frame mới khác old plane; old plane bằng frame mới thì cả màn; vùng trống thì không vẽ), x, y, rộng, cao (2 byte mỗi số). Chỉ đường raw |
| `05` | SUBMIT | slot, intent (0 `AUTO`, 1 `TEXT`, 2 `UI`, 3 `PAGE`), cờ (bit 0 hoãn cleanup, bit 1 trước khi ngủ). Đường raw bỏ intent và cờ, vẽ theo gói DRAW gần nhất |
| `06` | PLAY | ms giữa hai frame (2), số vòng (0 = mãi), intent, cờ, số slot, các slot |
| `07` | STOP | trống |
| `08` | POWER_OFF | trống: tắt analog power ngay; 4 byte: số ms không vẽ thì tự tắt (mặc định 4000, 0 = chỉ tắt bằng gói này). Chỉ đường raw |
| `09` | PROFILE_LOAD | gói profile `.inkw` do `tools/profile_gen.py` sinh. Sai mã nhận dạng, phiên bản, độ dài hoặc CRC thì trả lỗi, giữ profile tạm cũ. Profile tạm là index cuối trong INFO |
| `0A` | SET_OPTIONS | options của `ink_display`, cùng bố cục với khối options trong INFO. Chỉ đường pipeline |
| `0B` | READ_TEMP | trống; trả TEMP. Chỉ đường raw (đường pipeline có nhiệt độ trong DRAWN của lần cleanup strong) |
| `0C` | OTP_DUMP | trống; trả các gói OTP rồi ACK. Chỉ đường raw |
| `0D` | SET_PANEL | byte 2 của lệnh PSR (`0x00`), D7–D5 phải bằng 0, gửi controller trước lần vẽ hoặc tắt nguồn sau; rồi (không bắt buộc) VCOM_DC thay cho của mọi profile (byte lệnh `0x82`, tối đa `7E`; `FF` = theo profile), có tác dụng từ lần vẽ sau; rồi (không bắt buộc) số frame từ lúc tắt source tới lúc tắt gate khi POF (lệnh PFS `0x03`, 1–4); rồi (không bắt buộc) hướng quét gate: 0 = quét lên (UD = 1, mặc định), 1 = quét xuống (UD = 0), có tác dụng từ lần vẽ sau; rồi (không bắt buộc) byte TCON (lệnh `0x60`): nibble cao S2G, nibble thấp G2S, mỗi khoảng hở (n + 1) × 4 × 650 ns, mặc định `22`, có tác dụng từ lần vẽ sau; rồi (không bắt buộc) byte 3 của BTST (lệnh `0x06`, pha C: tăng áp khi đang chạy; bit 5–3 độ mạnh, bit 2–0 thời gian tắt tối thiểu của GDR), mặc định `17`, có tác dụng từ lần bật analog power sau; rồi (không bắt buộc) PSR byte 1 thay thế (RES, REG, KW/R, UD, SHL, SHD_N, RST_N), `00` = theo driver, có tác dụng từ lần vẽ sau. REG = 0 thì controller dùng sóng OTP chọn theo cảm biến nhiệt thay cho profile đã nạp. Đổi hướng quét thì ảnh lật trên dưới và RAM không còn khớp với panel, nên lần vẽ sau phải là cả màn với old plane đảo bit. Máy khởi động lại thì về `0A`, theo profile, 1 frame, quét lên, TCON `22`, BTST pha C `17`, PSR byte 1 theo driver. Cả hai đường |
| `0E` | BUTTON_EMULATE | nút (1 = A, 2 = B, 3 = AB), hành động (0 nhả, 1 nhấn, 2 click, 3 double click, 4 hold), thời gian ms (2 byte little-endian; 0 = mặc định). Giả lập nút bấm cho AI agent và test tự động. Cả hai đường |

Khối options (trong SET_OPTIONS và INFO): số thứ tự bản ghi là giá trị enum. Trước hết 4 bản ghi draw theo
`ink_display_intent_t` (`AUTO`, `TEXT`, `UI`, `PAGE`; `AUTO` không dùng), rồi 6 bản ghi cleanup theo
`ink_display_cleanup_t` (`INVALID`, `STRONG`, `FAST`, `EDGE_CENTER`, `EDGE_NEIGHBOR`, `LIGHT`; `INVALID` không dùng). Mỗi bản ghi:
số band, 9 band (°C có dấu 1 byte, id profile 2 byte), vào chế độ partial, `scan_outside_window` (bit `PT_SCAN`), drive border,
`debt_weight`, `marks_edges` (bản ghi cleanup ghi 0 hai trường này). Sau đó: `auto_ui_percent`, `debt_light`, `debt_page`,
`debt_ceiling`, `debt_ceiling_max` (1 byte mỗi số), rồi `active_ms`, `edge_ms`, `light_ms`, `strong_ms`, `power_off_ms`
(4 byte mỗi số).

Gói máy gửi lên:

| Loại | Tên | Payload |
|---|---|---|
| `80` | ACK | mã lỗi (4) |
| `81` | INFO | đường, rộng (2), cao (2), x_align, số slot, số profile (gồm profile tạm ở cuối), bản firmware (độ dài + chữ), rồi mỗi profile: id (2), thời gian một lần vẽ ms (2), nguồn (0–8 = TR0–TR8, 255 = vẽ tay), tên (độ dài + chữ); rồi khối options đang dùng; rồi byte 2 của PSR, VCOM_DC thay thế, số frame PFS, hướng quét gate (1 = quét xuống), byte TCON, byte 3 của BTST và PSR byte 1 thay thế đang đặt |
| `82` | DRAWN | slot, việc đã làm (`ink_display_action_t`), `intent` (255 khi không phải DRAW), nhiệt độ, nhiệt độ hợp lệ, debt lớn nhất, debt ceiling, frame (1 = lần vẽ một frame đã giao, 0 = cleanup hoặc tắt nguồn), id profile (2), `cleanup` (1; `INVALID` = 0 khi không phải CLEANUP), 0 (1), lỗi (4), x, y, rộng, cao (2 mỗi số), số điểm có edge flag, input ms, queue ms, decide ms, spi µs, panel µs, post-scan µs, bật nguồn µs (4 mỗi số) |
| `83` | GESTURE | modifier, nút, loại gesture, 0, thời điểm edge ms (4) |
| `84` | TEMP | °C, 0, 0, 0, lỗi (4) |
| `85` | LOG | một dòng chữ |
| `86` | OTP | offset (2), dữ liệu đọc bằng lệnh `0xA2`; nhiều gói cho tới hết band TR8 |

Việc đã làm trong DRAWN theo `ink_display_action_t`: 0 `INVALID`, 1 `NONE` (không vẽ), 2 `DRAW` (vẽ theo draw preset của
`intent`, hoặc vẽ raw), 3 `CLEANUP` (loại cleanup nằm ở byte `cleanup`), 4 `POWER_OFF` (tắt nguồn).

Chuỗi ảnh: frame sau được giao sau khi frame trước vẽ xong cộng thêm số ms của PLAY, nên không frame nào bị drop. Ở đường
pipeline, gói DRAWN của các cleanup không tính là "frame trước vẽ xong".

### Bài đo

Mỗi bài trả lời một câu hỏi mà thiết kế đang chờ số đo. Bài chạy trên trang, kết quả chấm điểm ghi vào results.

| Bài | Đường | Trả lời |
|---|---|---|
| Loại bóng mờ | raw | Ô lật 10 lần cùng chỗ bằng một profile: bóng là viền, xám trong vùng, hay cả màn xám dần |
| Cleanup theo edge flag | pipeline | Lật ô với intent `UI`, nghỉ: cleanup edge center và edge neighbor có xoá viền không; số điểm có cờ so với viền thấy trên ảnh chụp; DRF cleanup dưới 100 ms không |
| Cleanup theo edge flag nhiều ô | pipeline | Như bài trên với nhiều ô khác cỡ, khác chỗ, lật lần lượt hoặc cùng lúc: cờ thưa có bỏ sót viền ở ô nhỏ, ô gần nhau không |
| PSR byte 2 và tắt nguồn | pipeline | Trang chữ có debt, nghỉ qua cleanup edge, cleanup light, tắt nguồn, với các giá trị PSR byte 2: hạt có tự xám sau khi tắt nguồn không, và có xám khi nguồn vẫn bật không |
| VCOM và vùng đứng yên | raw | Nửa trắng, nửa đen đứng yên, ô giữa lật nhiều lần với window cả màn, quét VCOM_DC từ `00` (−0,1 V) tới `13` (−2,0 V) PSR byte 2 (có TIEG 0) và PFS 1–4 frame: vùng đứng yên có đổi màu dần trong lúc chạy, lúc tắt nguồn và sau đó không |
| Chữ dưới lớp đè | raw | Ô đen đè lên trang chữ: chữ cũ còn hiện vì điểm đen giữ đen không được kéo (LUT KK 0 V), vì đen cũ đã nhạt, hay vì điện trường lan từ điểm bên cạnh; vẽ ô bằng `TEXT` có đủ không |
| Lớp đè và `REDRAW_WINDOW` | pipeline | Hai ô đen đè lên trang chữ, một ô không cờ, một ô có cờ `REDRAW_WINDOW`: cờ có xoá chữ cũ trong ô không, và cleanup fast có chỉ kéo window của ô không |
| Chữ cho `TEXT` | raw | Trang chữ cuộn bằng profile chọn: profile nào cho chữ rõ nhất trong 0,3–0,4 s |
| Cleanup stage | pipeline | Lật nhiều lần rồi nghỉ qua cả ba bậc: đúng thứ tự, đúng thời điểm, đúng debt sau mỗi bậc |
| Thời gian policy | pipeline | Đổi cả màn liên tục: đoạn decide (policy, `pixel_state` trong PSRAM, old plane) mất bao lâu |
| Cleanup strong mạnh hơn | raw | Lật hai ô đen trên trang chữ nhiều lần bằng profile `UI`, rồi chạy profile cleanup strong cần thử lên trang chữ có khối đen ở dưới: profile nào xoá hết bóng ô và chữ cũ trong khối đen với DRF dưới 4 s. Ứng viên ở `tools/display_bench/profiles/cleanup_strong.json` (bảng TR4 chèn thêm chu kỳ lắc, giống hệt nhau ở cả bốn bảng nên mọi pixel lắc cùng chiều), nạp bằng gói PROFILE_LOAD |
| Khối đen theo thứ tự quét | raw | Hai khối đen trên nền trắng, khối A ở các hàng RAM đầu, khối B ở các hàng RAM cuối, vẽ cả màn bằng một profile: điểm đen có bị kéo nhạt bởi các hàng trắng quét sau nó trên cùng đường source không (khối A nhạt dần, khối B đều), có theo hàng RAM khi đổi UD không, và frame rate (`tr4_75hz`) có giảm không |
| Độ đen theo số điểm đen trên hàng | raw | Cột đen chạy suốt chiều cao, một nửa số hàng đen kín, nửa kia trắng: điểm đen có nhạt hơn khi cả hàng đen (nguồn VSH sụt theo số điểm cần kéo về đen) không, và có theo vị trí không |
| Ô đen dưới vùng mẫu | raw | Vùng trên tô trắng, đen, sọc dọc, sọc ngang hoặc bàn cờ; ô đen sát cáp FPC ở dưới: ô đen có nhạt khi các hàng quét trước làm đường source đổi mức nhiều không, và độ mạnh tăng áp (BTST) có sửa được không |
| `PT_SCAN` | raw | Vẽ ô A, rồi vẽ ô B ở chỗ khác với `PT_SCAN` 1 và 0: ô A có bị kéo theo không |
| Border | raw | Vẽ cả màn trong và ngoài chế độ partial, có và không kéo viền: border có đổi không |
| Nhiệt độ | raw | Đọc cảm biến nhiều lần giữa các lần vẽ (REG = 1): số có hợp lý so với nhiệt kế không |
| Giữ nguồn | raw | Vẽ, giữ nguồn bật trong N giây rồi tắt: đo dòng bằng đồng hồ ở hai trạng thái |
| Lật ô, Đảo cả màn, Quét cỡ window, Thang xám, Độ trễ nút | cả hai | Các bài cũ: lượt ghi RAM, độ đen trắng, thời gian theo cỡ window, dither, độ trễ từ lúc bấm |

### Kết quả chấm điểm

Mỗi lần chấm là một dòng JSON trong `tools/display_bench/results/<ngày>.jsonl`, gồm: điểm 1–5 (độ đen, độ trắng, bóng mờ,
viền, chớp, tổng thể), ghi chú, bài test, đường vẽ, profile (id, tên), options của đường pipeline, nhiệt độ, bản
firmware, và thời gian (trung bình, nhỏ nhất, lớn nhất của các gói DRAWN trong lần chạy, tách theo việc đã làm). File
này commit được, để so về sau.

### Chạy

1. Nạp firmware test trong terminal ESP-IDF: `idf.py -C tools/display_bench/firmware -p COM4 flash`.
2. Đóng `idf.py monitor` nếu đang mở: mỗi lúc chỉ một chương trình mở được cổng COM.
3. `python tools/display_bench/bench_server.py`, rồi mở `http://localhost:8253` trên Chrome hoặc Edge.
4. Test xong: `idf.py -p COM4 flash` để nạp lại firmware chính.

### Giả lập nút bấm cho AI agent và test tự động

AI agent hoặc script tự động có thể điều khiển nút bấm thay người qua công cụ dòng lệnh serial:

```powershell
# Click nút A hoặc B (mặc định giữ 50 ms rồi nhả, trả về JSON cho AI agent)
python tools/display_bench/button_cli.py click A --json
python tools/display_bench/button_cli.py click B --json

# Double click
python tools/display_bench/button_cli.py double A --json

# Nhấn giữ (HOLD) trong 800 ms
python tools/display_bench/button_cli.py hold A --duration 800 --json

# Tổ hợp hai nút A+B (combo)
python tools/display_bench/button_cli.py combo --json

# Nhấn giữ liên tục (down) và nhả (up)
python tools/display_bench/button_cli.py down A
python tools/display_bench/button_cli.py up A

# Lắng nghe các sự kiện (GESTURE, DRAWN, LOG) từ máy trong 5 s
python tools/display_bench/button_cli.py listen --seconds 5 --json
```

Cổng COM được tự động dò tìm (ESP32-S3 USB-Serial-JTAG). Có thể chỉ định cổng qua `-p / --port COM4`.
