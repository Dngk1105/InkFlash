# Tài liệu InkFlash

Thư mục này là nguồn sự thật cho thông số phần cứng và thiết kế firmware. Code trong `components/bsp` và `components/drivers` phải khớp với các file ở đây.

## Quy ước ghi thông số

Mỗi thông số ghi kèm nguồn và trạng thái. Ví dụ:

| Thông số | Giá trị | Nguồn | Trạng thái |
|---|---|---|---|
| Độ phân giải màn | 240×416 | Trang sản phẩm WeAct (link) | Chưa xác nhận |

- **Nguồn**: datasheet (tên file + trang), link web, hoặc "đo thực tế".
- **Trạng thái**: `Đã xác nhận` (đã đối chiếu datasheet hoặc đo) hoặc `Chưa xác nhận`. Giá trị trong ví dụ trên chỉ để minh hoạ cách ghi.

## Mục lục

### Phần cứng (`hardware/`)
- [display-weact-3.7.md](hardware/display-weact-3.7.md): màn e-paper WeAct 3.7" (chip điều khiển, độ phân giải, chân).
- [pinmap.md](hardware/pinmap.md): bảng GPIO cho breadboard (GĐ0) và PCB.
- [power.md](hardware/power.md): pin, sạc BQ25180, LDO RT9080, công tắc tải TPS22916.
- [parts.md](hardware/parts.md): IMU, audio, microSD và các linh kiện khác.

### Firmware (`firmware/`)
- [architecture.md](firmware/architecture.md): kiến trúc v2: các tầng, luật phụ thuộc và cách ép, sự kiện/lệnh, vòng đời app, lộ trình.
- [partitions.md](firmware/partitions.md): bảng phân vùng flash 16MB.

### Quyết định kiến trúc (`adr/`)
- [0001](adr/0001-ngon-ngu-c11.md): viết bằng C11.
- [0002](adr/0002-event-bus-esp-event.md): event bus dùng `esp_event`.
- [0003](adr/0003-cap-phat-dong.md): cho phép cấp phát động, có quy tắc.
- [0004](adr/0004-vong-doi-app-task.md): vòng đời app và task.

### Bản demo cũ
Code và tài liệu bản demo (driver UC8253/BNO055/I2S, benchmark) nằm ở tag git `demo-v0`.
Xem lại một file: `git show demo-v0:docs/firmware/demo.md`.

### Datasheet
- Lưu file PDF vào `datasheets/`, đặt tên theo mã linh kiện (ví dụ `BQ25180.pdf`).
