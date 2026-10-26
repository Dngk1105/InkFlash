# Phiên thức

## Vấn đề

Máy ngủ sâu để dòng ngủ thấp. Ngủ sâu trên ESP32 làm mất RAM và PSRAM; bấm nút thì chip khởi động lại từ đầu.
Người dùng vẫn phải thấy máy về đúng màn, đúng thẻ trước khi ngủ, và màn không chớp.

## Cách xử lý

### Mỗi lần thức là một phiên

| Trạng thái | Sang trạng thái | Khi |
|---|---|---|
| Ngủ sâu | Khởi động | Bấm nút |
| Khởi động | Khôi phục | Xong khởi động |
| Khôi phục | Hoạt động | Đọc xong snapshot |
| Hoạt động | Lưu | Hết giờ chờ, hoặc chọn Ngủ trong menu |
| Lưu | Ngủ sâu | Lưu xong, màn nền vẽ xong |

Máy trạng thái này nằm trong khối `session`, viết bằng C thuần: nhận sự kiện (nút, hết giờ chờ, pin yếu, vẽ xong),
trả về hành động (lưu snapshot, vẽ màn nền, ngủ). Phần gọi phần cứng nằm ở `system`.

### Snapshot trong RTC memory

RTC memory là vùng RAM nhỏ (8 KB) vẫn giữ dữ liệu khi ngủ sâu.

- Mỗi khối xin một ô riêng, kích thước cố định, có số version.
- Thức dậy thấy version của ô nào không khớp thì chỉ bỏ ô đó; khối đó khởi động như mới, khối khác không ảnh hưởng.
- App lưu ngăn xếp scene và vài byte trạng thái (ví dụ mã thẻ) vào ô của mình.

### Đi ngủ

1. Mỗi khối ghi snapshot vào ô của mình.
2. Vẽ màn nền: số thẻ cần ôn, mức pin.
3. Chờ task màn vẽ xong.
4. Ngủ sâu, nút được cấu hình để đánh thức.

App có thể xin giữ máy thức (ví dụ đang phát âm thanh), có giới hạn thời gian.

### Thức dậy

1. Đọc nguyên nhân thức. Thức từ ngủ sâu thì dùng kết quả dò phần cứng đã lưu, không dò lại.
2. Đọc snapshot.
3. Nạp màn nền vào controller làm "ảnh cũ" mà không làm mới màn, rồi vẽ màn cần hiện bằng partial refresh. Màn không chớp.

Lần khởi động nguội (cấp điện lần đầu) thì dò phần cứng, không có snapshot, máy vào menu.
