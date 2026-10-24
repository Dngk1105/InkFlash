# Module e-paper WeAct 3.7": bộ nhớ ảnh và lệnh controller

Panel GDEY037T03, controller UC8253. Nguồn: `GxEPD2_370_GDEY037T03.cpp`, **chưa đối chiếu datasheet UC8253**.

## Bộ nhớ ảnh

Controller có hai vùng ảnh: `0x13` là ảnh mới, `0x10` là ảnh cũ. Partial refresh so sánh hai vùng này để biết điểm
nào cần đổi, nên sau mỗi lần làm mới phải ghi ảnh mới vào vùng ảnh cũ.

Sau khi controller reset hoặc ngủ sâu, vùng ảnh cũ không còn khớp với ảnh đang hiện trên panel. Bản demo xử lý bằng
cách xoá trắng toàn màn (màn chớp). Có thể thay bằng cách nạp lại đúng ảnh đang hiện vào vùng `0x10` rồi mới partial
refresh; cách này chưa thử trên máy.

## Lệnh

| Bước | Lệnh |
|---|---|
| Khởi tạo | `0x00` + `1E 0D` (soft reset), rồi `0x00` + `1F 0D` |
| Ghi vùng ảnh | `0x91` (vào partial), `0x90` + cửa sổ (x, xe, y, ye, `01`), `0x13` hoặc `0x10` + dữ liệu, `0x92` (ra partial) |
| Full refresh | `E0 02`, `E5 5A`, `50 97`, `04` (bật nguồn), `12` (refresh), `02` (tắt nguồn) |
| Partial refresh | `E0 02`, `E5 6E`, `50 D7`, `04`, `12`, `02` |
| Ngủ sâu | `07` + `A5` |
