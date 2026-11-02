# Công cụ test màn

## Vấn đề

Muốn chọn bảng sóng, so bóng mờ hay đo tốc độ lật thì phải gửi ảnh tuỳ ý lên màn, chạy chuỗi ảnh với tốc độ chỉnh được,
rồi chấm điểm bằng mắt và lưu lại để so. Firmware chính không nên chứa phần này: lệnh chỉnh tay, thống kê và đường gửi
ảnh làm code dài và rối, trong khi người dùng không bao giờ cần.

## Cách xử lý

Ví dụ: cần biết profile `lut10f50` có để lại bóng mờ sau 50 lần lật không. Nạp firmware test, mở trang công cụ trên
Chrome, chọn bài "ô cờ bóng mờ", chọn profile partial `lut10f50`, chạy. Xong thì chấm điểm 1–5 cho bóng mờ; trang ghi
một dòng gồm điểm, profile, nhiệt độ và thời gian lật vào `tools/display_bench/results/`.

### Hai phần, tách khỏi firmware chính

| Phần | Nằm ở | Việc |
|---|---|---|
| Firmware test | `tools/display_bench/firmware/` | Project ESP-IDF riêng, dùng lại component của firmware chính (`ink_bsp`, driver, `ink_display`, `ink_display_policy`, `ink_input`). Giữ 64 slot ảnh trong PSRAM, vẽ theo lệnh của PC, báo thời gian từng lần vẽ, báo cử chỉ nút |
| Trang web | `tools/display_bench/web/` | Chạy trên Chrome hoặc Edge, nói chuyện với máy qua Web Serial (API mở cổng COM từ trang web). Xử lý ảnh, bài test, chuỗi ảnh, chấm điểm |
| Script phục vụ | `tools/display_bench/bench_server.py` | Thư viện chuẩn của Python. Phục vụ trang ở `http://localhost:8253` (Web Serial chỉ chạy trên `localhost`), ghi kết quả chấm điểm, liệt kê ảnh mẫu trong `images/` |

Firmware test nạp đè vào cùng phân vùng app với firmware chính; test xong thì nạp lại firmware chính. `tools/build_all.py`
build cả firmware test, nên component đổi API mà tool chưa sửa là build báo lỗi ngay.

Firmware chính chỉ có hai chỗ nối dùng chung với tool, và cả hai đều là API của sản phẩm: `ink_display_set_options` (app
Cài đặt cũng gọi) và callback `on_drawn` (luồng đi ngủ cũng chờ). Hàm duy nhất chỉ tool gọi là `ink_bsp_display_tune`,
vì chỉnh tay LUT cần biết chip.

### Hai đường vẽ

| Đường | Vẽ thế nào | Dùng khi |
|---|---|---|
| raw | Task bench gọi thẳng `draw` của driver: tự chọn full, partial, hoặc để policy chọn; window tuỳ ý; mặt phẳng cũ là ảnh đang hiện, một slot khác, hoặc khung mới đảo bit; đẩy lại viền tuỳ chọn | So bảng sóng, thử window, thử mặt phẳng cũ |
| pipeline | Chép slot vào khung mượn của `ink_display` rồi giao, như app thật: policy tự chọn, đếm bóng mờ, chọn profile theo nhiệt độ | Kiểm hành vi thật, đo từ lúc bấm nút |

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
| `04` | DRAW | slot, kiểu (0 policy, 1 partial, 2 full), nguồn mặt phẳng cũ (0 đang hiện, 1 slot, 2 đảo bit), slot cũ, độ rộng viền, 0, x, y, rộng, cao (2 byte mỗi số; rộng 0 = cả màn). Chỉ đường raw |
| `05` | SUBMIT | slot. Đường raw vẽ theo kiểu của gói DRAW hay PLAY gần nhất |
| `06` | PLAY | ms giữa hai khung (2), số vòng (0 = mãi), kiểu, độ rộng viền, số slot, các slot |
| `07` | STOP | trống |
| `08` | SELECT_PROFILE | slot màn (0 full, 1 partial), index profile |
| `09` | TUNE | slot màn, số khung, Hz (2), khung giữ, số lần quét, giữ nguồn |
| `0A` | SET_OPTIONS | profile full, profile partial, ngưỡng partial (2), độ rộng viền. Chỉ đường pipeline |
| `0B` | READ_TEMP | trống; trả TEMP. Chỉ đường raw (đường pipeline có nhiệt độ trong DRAWN của lần full) |

Gói máy gửi lên:

| Loại | Tên | Payload |
|---|---|---|
| `80` | ACK | mã lỗi (4) |
| `81` | INFO | đường, rộng (2), cao (2), x_align, số slot, số profile, profile full mặc định, profile partial mặc định, bản firmware (độ dài + chữ), rồi mỗi profile: thời gian điển hình ms (2), tên (độ dài + chữ) |
| `82` | DRAWN | slot, cách vẽ (0 không vẽ, 1 partial, 2 full), profile, nhiệt độ, nhiệt độ hợp lệ, 0, lỗi (4), x, y, rộng, cao (2 mỗi số), input ms, queue ms, diff ms, spi µs, panel µs, after µs (4 mỗi số) |
| `83` | GESTURE | modifier, nút, loại cử chỉ, 0, thời điểm edge ms (4) |
| `84` | TEMP | °C, 0, 0, 0, lỗi (4) |
| `85` | LOG | một dòng chữ |

Chuỗi ảnh: khung sau được giao sau khi khung trước vẽ xong cộng thêm số ms của PLAY, nên không khung nào bị bỏ.

### Kết quả chấm điểm

Mỗi lần chấm là một dòng JSON trong `tools/display_bench/results/<ngày>.jsonl`, gồm: điểm 1–5 (độ đen, độ trắng, bóng mờ,
viền, chớp, tổng thể), ghi chú, bài test, đường vẽ, profile và thông số chỉnh tay, độ rộng viền, nhiệt độ, bản firmware,
và thời gian (trung bình, nhỏ nhất, lớn nhất của các gói DRAWN trong lần chạy). File này commit được, để so về sau.

### Chạy

1. Nạp firmware test trong terminal ESP-IDF: `idf.py -C tools/display_bench/firmware -p COM4 flash`.
2. Đóng `idf.py monitor` nếu đang mở: mỗi lúc chỉ một chương trình mở được cổng COM.
3. `python tools/display_bench/bench_server.py`, rồi mở `http://localhost:8253` trên Chrome hoặc Edge.
4. Test xong: `idf.py -p COM4 flash` để nạp lại firmware chính.
