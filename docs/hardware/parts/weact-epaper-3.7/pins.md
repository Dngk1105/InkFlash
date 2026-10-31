# Module e-paper WeAct 3.7": chân và đầu nối

Có ba chỗ cần phân biệt: đầu 8 chân nối ra ngoài (nơi ESP32 đấu dây), điện trở nối tiếp trên tín hiệu, và đầu FPC 24 chân nối vào panel.

## Ví dụ

Đấu ESP32 với module: chân BUSY của module vào một GPIO đầu vào, các chân RES, D/C, CS vào GPIO đầu ra, SCL vào chân SCK của SPI, SDA vào chân MOSI của SPI. Cấp 3,3 V vào VCC và nối GND. Số GPIO cụ thể của bo BB0 nằm ở `boards/bb0.md`.

## Đầu 8 chân ra ngoài

Cùng 8 tín hiệu có hai chỗ hàn song song: hàng chân 2 × 4 bước 2,54 mm (`P1`) và đầu Wafer 1,25 mm 8 chân (`J2`). Đánh số hai nơi trùng nhau.


| Chân | Tên in trên module | Hướng (tính từ ESP32) | Chức năng                                                   | Chân UC8253 |
| ------- | ---------------------- | --------------------------- | --------------------------------------------------------------- | -------------- |
| 1     | BUSY                 | Vào                      | Mức thấp = controller đang bận, không được gửi lệnh | BUSY_N       |
| 2     | RES                  | Ra                        | Reset phần cứng, tích cực mức thấp                      | RST_N        |
| 3     | D/C                  | Ra                        | Thấp = byte kế là lệnh; cao = byte kế là dữ liệu      | DC           |
| 4     | CS                   | Ra                        | Chọn chip, tích cực mức thấp                             | CSB          |
| 5     | SCL                  | Ra                        | Xung nhịp SPI (SCK)                                          | SCL          |
| 6     | SDA                  | Ra                        | Dữ liệu SPI (MOSI)                                          | SDA          |
| 7     | GND                  |                           | Mass                                                          |              |
| 8     | VCC                  |                           | Nguồn vào của module (xem `electrical.md`)                 |              |

Ghi chú:

- Không có chân MISO. Controller trả dữ liệu đọc trên chính chân SDA (UC8253 tr.41–42). ESP32 đọc được bằng device SPI bán song công 3 dây, để chân MOSI vừa ghi vừa đọc; đã chạy trên BB0 (2026-10-30, `interface.md`).
- Bản vẽ hình dạng in nhãn theo cột dọc, số chân 1 và 2 nằm ở đầu dưới của hàng 2 × 4, số 7 và 8 ở đầu trên.

## Điện trở nối tiếp trên tín hiệu

Sáu tín hiệu BUSY, RES, D/C, CS, SCL, SDA đều đi qua điện trở 100 Ω nối tiếp (R3–R8, cỡ 0402) giữa đầu 8 chân và panel. Điện trở này không đổi logic; mục đích (nhiều khả năng là giảm nhiễu và đỉnh dòng) là suy luận, sơ đồ không ghi.

## Chọn SPI 3 dây hay 4 dây

Chân BS của controller chọn kiểu giao tiếp. Trên module, BS đi qua điện trở 5,1 kΩ (R9) tới hai cầu hàn:


| Cầu hàn | Nối BS tới  | Kiểu SPI                          |
| ----------- | --------------- | ------------------------------------ |
| SB1       | GND           | 4 dây (BS thấp), dùng chân D/C |
| SB2       | VDD33 (3,3 V) | 3 dây (BS cao), dữ liệu 9 bit   |

Các ví dụ của WeAct điều khiển chân D/C nên module chạy 4 dây. Trạng thái hàn mặc định của SB1/SB2 trên module thật chưa kiểm tra bằng đồng hồ: `Đã xác nhận`. Nếu dùng 3 dây thì UC8253 yêu cầu chân DC nối GND (UC8253 tr.7), mà trên module chân D/C đi thẳng ra đầu 8 chân nên ESP32 phải giữ D/C ở mức thấp.

## Đầu FPC 24 chân vào panel (J1)

`J1` là đầu FPC bước 0,5 mm, 24 chân. Sơ đồ WeAct đánh số J1 ngược chiều với bảng chân của panel: chân J1 = 25 − chân panel. Ví dụ: SDA là chân 14 của panel, nên ở J1 là chân 11.


| Chân panel | Tín hiệu panel | Chân J1 | Nối tới trên module                                               |
| ------------- | ------------------ | ---------- | ---------------------------------------------------------------------- |
| 2           | GDR              | 23       | Cổng MOSFET Q1 của mạch tăng áp                                 |
| 3           | RESE             | 22       | Điện trở cảm biến dòng R2 của mạch tăng áp                 |
| 8           | BS               | 17       | Chọn SPI 3/4 dây (SB1/SB2)                                         |
| 9           | BUSY_N           | 16       | BUSY                                                                 |
| 10          | RST_N            | 15       | RES                                                                  |
| 11          | DC               | 14       | D/C                                                                  |
| 12          | CSB              | 13       | CS                                                                   |
| 13          | SCL              | 12       | SCL                                                                  |
| 14          | SDA              | 11       | SDA                                                                  |
| 15, 16      | VDDIO, VCC       | 10, 9    | VDD33 (3,3 V)                                                        |
| 21          | VGH              | 4        | Rail điện áp cực cổng dương (net E_VGH), có tụ lọc về GND |
| 23          | VGL              | 2        | Rail điện áp cực cổng âm (net E_VGL), có tụ lọc về GND     |
| 1, 4        | NC               | 24, 21   | Để hở                                                             |

Các chân còn lại (VCOM, VSL, VSH1, VSH2, VPP, VDD, VSS, GND) nối tụ lọc 1 µF về GND hoặc mass; bảng đầy đủ của hãng ở Panel tr.7 và sơ đồ ở `source/module-schematic-4.2-3.7.pdf`. File sơ đồ có tiêu đề "4.2 Inch" vì WeAct dùng chung một trang cho hai cỡ 4,2" và 3,7".

Đối chiếu gốc: `source/module-schematic-4.2-3.7.pdf` (1 trang); `source/module-board-shape-3.7.pdf` tr.1; `source/panel-e037a75-a0-spec.pdf` tr.6–8; `source/uc8253-datasheet.pdf` tr.6–7.
