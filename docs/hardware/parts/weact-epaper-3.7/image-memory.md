# Module e-paper WeAct 3.7": bộ nhớ ảnh và định dạng dữ liệu

Controller có hai vùng RAM ảnh, mỗi vùng 1 bit cho mỗi điểm. File này nói ảnh nằm trong RAM thế nào, bit 1 nghĩa là đen hay trắng, và cách ghi một cửa sổ nhỏ (partial window).

## Ví dụ: kích thước một khung hình

Màn rộng 240 điểm, cao 416 điểm. Tám điểm gói vào một byte nên mỗi hàng là 30 byte, cả màn là 30 × 416 = 12 480 byte cho một vùng RAM. Ở SCL 4 MHz, đẩy một vùng mất ít nhất 12 480 × 8 / 4 000 000 ≈ 25 ms (chưa kể thời gian nâng/hạ CS); hai vùng khoảng 50 ms.

## Hai vùng RAM

| Lệnh | Vùng (chế độ KW, đen trắng) | Ghi chú |
|---|---|---|
| `0x10` (DTM1) | Ảnh cũ (OLD) | Ảnh đang hiện trên màn |
| `0x13` (DTM2) | Ảnh mới (NEW) | Ảnh muốn hiện |

Ở chế độ KWR (đen/trắng/đỏ), hai lệnh này mang ý nghĩa khác: `0x10` là lớp đen trắng, `0x13` là lớp đỏ (UC8253 tr.20–21). Module này dùng chế độ KW.

Khi làm mới, controller lấy từng điểm một cặp bit (mới, cũ) và chọn một trong bốn bảng sóng: trắng giữ trắng, đen giữ đen, đen thành trắng, trắng thành đen (xem `waveform.md`). Nhờ vậy partial refresh chỉ đổi những điểm cần đổi.

Datasheet không mô tả việc controller tự chép ảnh mới sang ảnh cũ sau khi làm mới (đã tìm ở tr.20–21, 29). Thư viện GxEPD2 và demo xử lý bằng cách ghi ảnh mới vào vùng ảnh cũ sau mỗi lần làm mới.

Sau khi controller reset hoặc ngủ sâu, vùng ảnh cũ không còn khớp với ảnh đang hiện trên panel. Demo xử lý bằng cách xoá trắng toàn màn (màn chớp). Có thể thay bằng cách nạp lại đúng ảnh đang hiện vào vùng `0x10` rồi mới partial refresh; cách này chưa thử trên máy.

## Bit 1 là đen hay trắng

Điều này do bit DDX trong lệnh `0x50` (CDI) quyết định, không phải do panel.

| DDX[0] (bit 4 của tham số CDI) | Bit 1 | Bit 0 | Giá trị CDI dùng bởi | Trạng thái |
|---|---|---|---|---|
| 1 (mặc định sau reset) | Trắng | Đen | GxEPD2: `0x97` full, `0xD7` partial | Đã xác nhận (UC8253 tr.29) |
| 0 | Đen | Trắng | Mã mẫu hãng: `0x47`, `0xC7` | Đã xác nhận (UC8253 tr.29; mã mẫu hãng) |

Với DDX[0] = 1, bảng chuyển trạng thái (NEW, OLD) như sau (UC8253 tr.29):

| NEW | OLD | Điểm đổi thế nào | Bảng sóng |
|---|---|---|---|
| 0 | 0 | Đen giữ đen | LUTKK |
| 1 | 1 | Trắng giữ trắng | LUTWW |
| 1 | 0 | Đen thành trắng | LUTKW |
| 0 | 1 | Trắng thành đen | LUTWK |

Với DDX[0] = 0 thì ngược lại: 0 là trắng, 1 là đen, các dòng đổi nhãn tương ứng. Khi đem mã mẫu của hãng sang dùng cùng thư viện khác, phải đảo bit ảnh cho khớp giá trị CDI đang đặt.

Bit DDX[1] = 1 chọn chế độ "chỉ có ảnh mới, không có ảnh cũ" (một vùng RAM); chưa thử trên module.

## Thứ tự byte

| Hạng mục | Quy định | Nguồn | Trạng thái |
|---|---|---|---|
| Số điểm gói trong một byte | 8; điểm thứ nhất ở bit cao nhất (D7) | UC8253 tr.20 | Đã xác nhận |
| Byte mỗi hàng | 30 (240 / 8) | Tính từ độ phân giải | Đã xác nhận |
| Số hàng (gate) | 416 | Panel tr.6 | Đã xác nhận |
| Thứ tự gửi | Từng hàng một, mỗi hàng 30 byte; mã mẫu hãng lặp 416 hàng × 30 byte | Mã mẫu hãng dòng 188–226 | Đã xác nhận |
| Kích thước một vùng RAM | 12 480 byte | Tính | Đã xác nhận |
| Điểm đầu tiên ứng với góc nào của màn | Phụ thuộc bit UD, SHL của lệnh `0x00` (mặc định: quét gate lên, dịch source sang phải) và cách panel nối dây | UC8253 tr.14 | Chưa xác nhận trên máy |

Độ phân giải do lệnh `0x61` (TRES) hoặc bit RES của lệnh `0x00` quyết định, và OTP của controller có thể đổi giá trị mặc định (UC8253 tr.51). Mã mẫu hãng không gửi `0x61` mà vẫn ghi đủ 416 hàng; nhiều khả năng OTP đã đặt sẵn 240 × 416, nhưng chưa xác nhận.

## Ghi một cửa sổ (partial window)

Bốn lệnh làm việc cùng nhau:

| Bước | Lệnh | Việc làm |
|---|---|---|
| 1 | `0x91` (PTIN) | Vào chế độ partial |
| 2 | `0x90` (PTL), 7 byte tham số | Đặt cửa sổ |
| 3 | `0x10` hoặc `0x13` + dữ liệu | Ghi chỉ phần trong cửa sổ |
| 4 | `0x92` (PTOUT) | Ra chế độ partial |

Bảy byte của `0x90` (UC8253 tr.37):

| Byte | Nội dung | Ghi chú |
|---|---|---|
| 1 | HRST[7:3]: cột bắt đầu | Ba bit thấp bằng 000, nên x bắt đầu là bội của 8 |
| 2 | HRED[7:3]: cột kết thúc | Ba bit thấp bằng 111 (mặc định `0x07`), nên x kết thúc là 8k + 7 |
| 3, 4 | VRST[8:0]: hàng bắt đầu | Tối đa 479 |
| 5, 6 | VRED[8:0]: hàng kết thúc | Phải lớn hơn VRST |
| 7 | PT_SCAN | 1 (mặc định): quét gate cả trong và ngoài cửa sổ; 0: chỉ quét trong cửa sổ |

Ví dụ: cửa sổ rộng hết màn (x từ 0 tới 239) và cao 100 hàng (y từ 100 tới 199): byte 1 = `0x00`, byte 2 = `0xEF` (239), VRST = 100, VRED = 199. Dữ liệu cần ghi là 30 × 100 = 3 000 byte cho mỗi vùng RAM.

Hệ quả: vùng partial luôn rộng theo bội 8 điểm theo trục ngang gốc (240 điểm). Khi xoay màn ngang (416 × 240) thì ràng buộc bội 8 áp lên trục dọc của hình xoay.

Đối chiếu gốc: `source/uc8253-datasheet.pdf` tr.14, 20–21, 29, 32, 37; `source/panel-vendor-init-uc8253.c` dòng 176–348.
