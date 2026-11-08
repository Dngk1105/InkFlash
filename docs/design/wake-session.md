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
| Active | Save | Hết giờ chờ, hoặc chọn Ngủ trong menu |
| Save | Deep sleep | Save xong, background frame vẽ xong |

State machine này nằm trong `session`, là core module: nhận event (nút, hết giờ chờ, pin yếu, vẽ xong), trả action
(lưu snapshot, vẽ background frame, ngủ). Phần gọi phần cứng nằm ở `system`.

### Snapshot trong RTC memory

RTC memory là vùng RAM nhỏ (8 KB) vẫn giữ dữ liệu khi deep sleep.

- Mỗi module xin một RTC slot riêng, cỡ cố định, có số version.
- Wake thấy version của RTC slot nào không khớp thì chỉ bỏ RTC slot đó; module đó khởi động như mới, module khác không
  ảnh hưởng.
- App lưu scene stack và vài byte state (ví dụ mã thẻ) vào RTC slot của mình.

### Đi ngủ

1. Mỗi module ghi snapshot vào RTC slot của mình.
2. Vẽ background frame: số thẻ cần ôn, mức pin. Frame này mang flag `BEFORE_SLEEP`: còn debt thì `display` vẽ nó bằng
   cleanup strong, rồi tắt analog power. Debt và edge flag của từng pixel nằm trong PSRAM, mất khi deep sleep, nên
   panel phải sạch trước khi ngủ.
3. Chờ display task báo vẽ xong và tắt analog power.
4. Vào deep sleep; nút được cấu hình để wake.

App xin được giữ máy thức (ví dụ đang phát âm thanh), có giới hạn thời gian.

### Wake

1. Đọc nguyên nhân wake. Wake từ deep sleep thì dùng kết quả dò phần cứng đã lưu, không dò lại.
2. Đọc snapshot.
3. Coi background frame là shown frame: máy đã vẽ nó trước khi ngủ, và panel e-ink giữ hình khi mất nguồn. Rồi vẽ màn
   cần hiện trên cả panel bằng preset của intent, với old plane là background frame. Màn không chớp. Mọi pixel bắt đầu
   với debt bằng 0.

Cold boot (cấp điện lần đầu) thì dò phần cứng, không có snapshot, máy vào menu.
