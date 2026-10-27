# Module e-paper WeAct 3.7": thuật ngữ

Từ viết tắt dùng trong thư mục này, xếp theo nhóm. Từ nào chưa có ở đây thì tìm trong datasheet UC8253.

## Ví dụ

Câu "DDX = 01 nên bit 1 là trắng" có ba từ cần biết: DDX (cực tính dữ liệu), bit 1/0 (dữ liệu ảnh) và trắng (trạng thái điểm ảnh). Bảng dưới giải thích từng từ.

## Màn hình

| Từ | Nghĩa |
|---|---|
| EPD, e-paper | Màn giấy điện tử: hiển thị bằng hạt mực mang điện, giữ ảnh không cần điện |
| AMEPD | Active Matrix Electrophoretic Display: e-paper điều khiển bằng ma trận TFT |
| Bi-stable | Hai trạng thái ổn định (đen, trắng), giữ nguyên khi tắt điện |
| TFT | Thin Film Transistor: transistor màng mỏng, mỗi điểm ảnh có một cái làm công tắc |
| Gate | Đường quét hàng của ma trận TFT (416 đường) |
| Source | Đường dữ liệu theo cột (240 đường) |
| Điểm ảnh, pixel | Một ô của màn; ở đây chỉ có đen hoặc trắng (1 bit) |
| Bóng mờ (ghosting) | Hình cũ còn mờ lại sau khi đã đổi sang hình mới |
| Frame, khung hình | Một chu kỳ quét toàn màn trong lúc làm mới (đơn vị thời gian của bảng sóng) |
| ESL | Electronic Shelf Label: nhãn giá điện tử, ứng dụng điển hình của controller này |
| FPC | Flexible Printed Circuit: cáp mạch dẻo nối panel với bo mạch |
| COG | Chip On Glass: chip controller dán thẳng trên kính |

## Điện áp

| Từ | Nghĩa |
|---|---|
| VCOM | Điện cực chung của toàn màn; điện trường mỗi điểm ảnh là hiệu giữa điện áp điểm đó và VCOM |
| VSH, VSL | Điện áp nguồn dương, âm cấp cho điểm ảnh khi đổi trạng thái đen hoặc trắng (±2,4 tới ±15 V) |
| VDHR | Điện áp nguồn dương cho điểm ảnh đỏ (module này không dùng) |
| VGH, VGL | Điện áp cực cổng dương, âm (±20 V) để bật hoặc tắt TFT |
| LDO | Bộ ổn áp tụt thấp (Low DropOut); ở đây là ME6231A33M3G, ra 3,3 V |
| Tăng áp, booster | Mạch nâng 3,3 V lên điện áp cao bằng cuộn cảm, MOSFET và diode |

## Giao tiếp

| Từ | Nghĩa |
|---|---|
| SPI 4 dây | SCL, SDA, CS và D/C |
| CSB, CS | Chọn chip, tích cực mức thấp |
| D/C | Phân biệt byte lệnh (thấp) và byte dữ liệu (cao) |
| BUSY, BUSY_N | Chân báo bận, mức thấp là đang bận |
| RES, RST_N | Chân reset phần cứng, tích cực mức thấp |
| BS | Chân chọn SPI 3 dây hoặc 4 dây |
| SB1–SB5 | Cầu hàn (solder bridge) trên module: SB1/SB2 chọn kiểu SPI, SB3/SB4/SB5 chọn đường cấp nguồn |
| Hsync | Một chu kỳ quét một hàng; đơn vị khoảng cách trong thanh ghi CDI |

## Controller và dữ liệu

| Từ | Nghĩa |
|---|---|
| UC8253 | Controller của UltraChip, tích hợp bộ định thời, tăng áp, SRAM và OTP |
| KW, KWR | Chế độ đen trắng (K = Kuro, đen; W = trắng) hoặc đen trắng đỏ (R) |
| OLD, NEW | Hai vùng RAM ảnh: ảnh đang hiện và ảnh muốn hiện |
| DDX | Hai bit trong lệnh `0x50` quy định bit 1 là đen hay trắng |
| LUT | Look-Up Table: bảng chứa dạng sóng điện áp cho từng kiểu chuyển trạng thái |
| LUTWW, LUTWK, LUTKW, LUTKK | Bảng cho điểm trắng → trắng, trắng → đen, đen → trắng, đen → đen |
| LUTC | Bảng sóng của VCOM |
| OTP | One-Time Programmable: bộ nhớ ghi một lần trong controller, chứa bảng sóng và giá trị mặc định |
| TR, TB | Dải nhiệt độ (Temperature Range) và ngưỡng nhiệt độ (Temperature Boundary) trong OTP |
| PSR, CDI, PTL, DRF... | Tên viết tắt của lệnh; bảng đầy đủ ở `commands.md` |
| Full refresh | Làm mới cả màn bằng bảng sóng đầy đủ, màn chớp |
| Partial refresh | Chỉ làm mới một cửa sổ, nhanh và không chớp cả màn, nhưng tích bóng mờ |
| Deep clean | Đảo cả màn đen rồi trắng trước khi vẽ, để xoá bóng mờ |
| Deep sleep | Chế độ ngủ sâu của controller, chỉ thoát bằng reset phần cứng |
