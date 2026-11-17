# Wake session

## Vấn đề

Máy dùng deep sleep để dòng ngủ thấp. Deep sleep trên ESP32-S3 làm mất RAM và PSRAM; bấm nút thì chip boot lại từ
đầu. Người dùng vẫn phải thấy máy về đúng màn, đúng thẻ trước khi ngủ, và màn không chớp.

## Cách xử lý

### Mỗi lần thức là một wake session

| Trạng thái | Sang trạng thái | Khi |
|---|---|---|
| Deep sleep | Boot | Bấm nút |
| Boot | Restore | Boot xong |
| Restore | Active | Đọc xong snapshot |
| Active | Save | Hết giờ chờ, hoặc chọn Ngủ trong Home |
| Save | Deep sleep | Save xong, background frame vẽ xong |

State machine này nằm trong `session`, là core module: nhận event (nút, hết giờ chờ, pin yếu, vẽ xong), trả action
(lưu snapshot, vẽ background frame, ngủ). Phần gọi phần cứng nằm ở `system`.

### Snapshot trong RTC memory

RTC memory là vùng RAM nhỏ vẫn giữ dữ liệu khi deep sleep. Snapshot nằm trong RTC slow memory (8 KB, `RTC_DATA_ATTR`);
RTC fast memory (8 KB) đang dùng làm heap.

- Mỗi module xin một RTC slot riêng, cỡ cố định. Snapshot chỉ chứa số, không chứa pointer hay địa chỉ RAM.
- Đầu mỗi RTC slot có header: magic, version, độ dài payload, CRC của payload.
- Wake thấy RTC slot nào sai magic, sai version, sai độ dài hoặc sai CRC thì chỉ bỏ RTC slot đó; module đó khởi động
  như mới, module khác không ảnh hưởng.
- Cỡ struct snapshot của mỗi module được kiểm bằng `static_assert` lúc biên dịch: không vượt cỡ RTC slot của module.
- App lưu scene stack (mỗi tầng `scene_id` và 8 byte scene state) và state cấp app vào RTC slot của mình.

### Reset reason quyết định có đọc snapshot không

`system` đọc reset reason ngay sau boot. Chỉ deep sleep wake mới đọc RTC slot.

| Reset reason | Xử lý |
|---|---|
| Deep sleep wake | Đọc RTC slot, dựng lại background frame, vẽ màn trước khi ngủ không chớp |
| Mọi reason khác: cấp điện, nút reset, panic, watchdog, brownout, `esp_restart()` | Cold boot: bỏ qua RTC slot, vào Home |

Máy chỉ vào deep sleep sau khi `display` báo đã tắt analog power, nên chỉ sau deep sleep wake thì panel mới chắc chắn
đang hiện background frame. Ví dụ: brownout xảy ra giữa lúc panel đang quét. Reset reason là brownout, nên máy cold
boot; `display` chưa biết panel đang hiện gì nên lần vẽ đầu là cleanup strong. Không cần cờ riêng đánh dấu "panel đang
vẽ".

Nạp firmware mới cũng gây reset khác deep sleep, nên snapshot luôn do chính firmware đang chạy ghi ra.

### Background frame dựng lại từ metadata

Background frame cỡ 240 × 416 pixel, 1 bit mỗi pixel, là 12 480 byte, không vừa RTC slow memory 8 KB. Vì vậy `system`
không lưu ảnh, chỉ lưu metadata vào RTC slot của mình: số thẻ cần ôn và mức pin đã đo lúc đi ngủ.

Wake xong, `system` vẽ lại background frame bằng chính hàm vẽ đã dùng trước khi ngủ, với metadata đọc từ RTC slot.
Cùng firmware, cùng hàm, cùng số thì ra cùng pixel. Mức pin lấy từ metadata, không đo lại. `system` đưa frame này cho
`display` qua `background` của `ink_display_start()`.

### Đi ngủ

1. Mỗi module ghi snapshot vào RTC slot của mình.
2. `system` đo mức pin, ghi metadata của background frame vào RTC slot của mình.
3. Vẽ background frame từ metadata đó. Frame này mang flag `BEFORE_SLEEP`: còn debt thì `display` vẽ nó bằng cleanup
   strong, rồi tắt analog power. Debt và edge flag của từng pixel nằm trong PSRAM, mất khi deep sleep, nên panel phải
   sạch trước khi ngủ.
4. Chờ report `POWER_OFF` của display tới main queue.
5. Vào deep sleep; nút được cấu hình để wake.

### Giữ máy thức

App gọi `keep_awake(ms)` để hoãn hết giờ chờ, ví dụ khi đang phát âm thanh. Mỗi lần gọi giữ tối đa 30 s; cần lâu hơn
thì gọi lại. App treo và không gọi lại thì máy ngủ sau tối đa 30 s.

### Wake

1. Đọc reset reason. Wake từ deep sleep thì dùng kết quả dò phần cứng đã lưu, không dò lại.
2. Đọc snapshot.
3. Dựng lại background frame từ metadata. Coi background frame là shown frame: máy đã vẽ nó trước khi ngủ, và panel e-ink giữ hình khi mất nguồn. Rồi vẽ màn
   cần hiện trên cả panel bằng preset của intent, với old plane là background frame. Màn không chớp. Mọi pixel bắt đầu
   với debt bằng 0.

Cold boot thì dò phần cứng, không đọc snapshot, máy vào Home.
