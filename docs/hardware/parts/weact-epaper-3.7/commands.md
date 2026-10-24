# Module e-paper WeAct 3.7": bộ nhớ ảnh và lệnh controller

Panel E037A75-A0 (240×416), controller tích hợp UC8253. Đã đối chiếu datasheet UC8253 và code mẫu WeAct.

## Bộ nhớ ảnh (Display SRAM)

Màn có độ phân giải gốc 240 (ngang) × 416 (dọc). Mỗi điểm ảnh chiếm 1 bit (0 = Đen, 1 = Trắng):
- Mỗi hàng: 240 bit = 30 byte.
- Toàn màn hình: 30 byte/hàng × 416 hàng = **12.480 byte**.

Controller UC8253 chứa hai vùng đệm ảnh riêng biệt trong SRAM:

| Lệnh nạp | Tên vùng đệm | Chức năng trong thuật toán làm mới |
|---|---|---|
| `0x10` (DTM1) | Vùng ảnh trước (Old Buffer) | Lưu dữ liệu điểm ảnh đang hiển thị trên panel. |
| `0x13` (DTM2) | Vùng ảnh sau (New Buffer) | Lưu dữ liệu điểm ảnh mới chuẩn bị vẽ. |

Khi chạy chế độ Partial refresh (làm mới từng phần), controller tự động so sánh từng bit giữa vùng `0x10` và `0x13`. Chỉ các điểm ảnh có trạng thái thay đổi mới được cấp xung điện di chuyển hạt mực, giúp quá trình làm mới diễn ra nhanh và không chớp màn.

> [!NOTE]
> Sau mỗi lần làm mới thành công, phần mềm phải đồng bộ dữ liệu: chép ảnh mới vào vùng `0x10` trước khi chuẩn bị cho lần vẽ kế tiếp.

## Bảng lệnh điều khiển UC8253

| Mã lệnh (Hex) | Tên lệnh | Tham số dữ liệu | Mô tả chức năng |
|---|---|---|---|
| `0x00` | **PSR** (Panel Setting) | 2 byte (`1F 0D`) | Cấu hình kích thước, chế độ quét và bật controller. |
| `0x01` | **PWR** (Power Setting) | 4-5 byte | Cấu hình điện áp nội cho khối kích nguồn (booster). |
| `0x02` | **POF** (Power OFF) | Không có | Tắt mạch cấp cao áp DC-DC sau khi màn đã quét xong. |
| `0x04` | **PON** (Power ON) | Không có | Bật mạch kích cao áp DC-DC. Chờ chân BUSY lên 1. |
| `0x06` | **BTST** (Booster Soft Start) | 3 byte | Cấu hình khởi động mềm cho mạch tăng áp. |
| `0x07` | **DSLP** (Deep Sleep) | 1 byte (`0xA5`) | Đưa màn vào chế độ ngủ sâu (tiêu thụ ~3 µA). |
| `0x10` | **DTM1** (Data Start 1) | 12.480 byte | Nạp dữ liệu ảnh vào vùng đệm trước (hoặc ảnh đen/trắng). |
| `0x11` | **DSP** (Data Stop) | Không có | Kết thúc nạp dữ liệu vào vùng đệm. |
| `0x12` | **DRF** (Display Refresh) | Không có | Kích hoạt chu kỳ quét hạt mực theo bảng sóng (LUT). |
| `0x13` | **DTM2** (Data Start 2) | 12.480 byte | Nạp dữ liệu ảnh vào vùng đệm sau (ảnh mới). |
| `0x50` | **CDI** (VCOM and Data Interval) | 1 byte (`0xC7` hoặc `0x97`) | Cấu hình điện áp VCOM và màu đường viền màn (Border). |
| `0x61` | **TRES** (Resolution Setting) | 3 byte (`00 F0 01 A0`) | Khai báo độ phân giải: 240 ngang (`0x00F0`), 416 dọc (`0x01A0`). |
| `0x90` | **PARTIAL_WIN** | 7 byte | Thiết lập tọa độ cửa sổ hình chữ nhật cần partial update. |
| `0x91` | **PARTIAL_IN** | Không có | Bắt đầu vào chế độ làm mới từng phần. |
| `0x92` | **PARTIAL_OUT** | Không có | Thoát khỏi chế độ làm mới từng phần. |

## Trình tự gửi lệnh cơ bản

1. **Khởi tạo và đánh thức**:
   - Kéo chân RES xuống 0 trong 50 µs rồi kéo lên 1.
   - Chờ chân BUSY lên mức 1.
   - Gửi lệnh `0x00` (PSR) kèm tham số cấu hình panel.
   - Gửi lệnh `0x04` (PON) để kích hoạt mạch nguồn cao áp, chờ BUSY lên 1.
2. **Nạp dữ liệu ảnh**:
   - Gửi lệnh `0x10` (DTM1) rồi truyền liên tiếp 12.480 byte dữ liệu qua SPI.
   - Nếu làm mới từng phần: gửi thêm lệnh `0x13` (DTM2) với dữ liệu mới.
3. **Kích hoạt quét**:
   - Gửi lệnh `0x12` (DRF).
   - Chờ chân BUSY giữ mức 0 rồi chuyển lên mức 1 (mất khoảng 2,3 ~ 2,8 giây với Full refresh).
4. **Tắt nguồn và ngủ sâu**:
   - Gửi lệnh `0x02` (POF) để ngắt nguồn cao áp, chờ BUSY lên 1.
   - Gửi lệnh `0x07` kèm byte `0xA5` để ngủ sâu.
