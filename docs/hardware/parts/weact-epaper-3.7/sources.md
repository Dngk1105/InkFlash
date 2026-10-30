# Module e-paper WeAct 3.7": danh mục tài liệu gốc

Mọi con số trong thư mục này truy ngược về một trong các file trong `source/`. File này nói mỗi file là gì, trang nào đáng đọc, và những gì không được copy vào repo.

## Ví dụ

Muốn biết chân BS chọn SPI mấy dây: `index.md` dẫn tới `pins.md`; ở cuối file đó ghi `uc8253-datasheet.pdf` tr.6–7. Mở trang 6 của PDF, tìm dòng "BS". Nếu vẫn chưa rõ thì mở `module-schematic-4.2-3.7.pdf` để xem R9, SB1, SB2.

## Tài liệu trong `source/`

Số trang là số trang PDF, cũng là số in ở chân trang.

| File | Là gì | Tác giả | Trang đáng đọc | Ghi chú |
|---|---|---|---|---|
| `uc8253-datasheet.pdf` | Datasheet controller UC8253 (75 trang), bản c_A, revision 0.6, 2020-10-13, "for TFT_module_Use_Only" | UltraChip | tr.3 tính năng; tr.6–7 chân; tr.8–13 bảng lệnh; tr.14–40 mô tả lệnh; tr.41–46 giao tiếp và nguồn; tr.47–52 OTP và nhiệt độ; tr.58–61 điện | Có text; bảng lệnh bị bể cột khi trích text |
| `gdey037t03-spec.pdf` | Spec panel GDEY037T03 (45 trang), bản có front-light và cảm ứng, revision 1.1 | Dalian Good Display | tr.5 tổng quan; tr.9–26 bảng lệnh (chép từ datasheet UC8253); tr.13 bảng điện áp VSH, VSL, VDHR; tr.29–31 giao tiếp, đọc qua SDA; tr.39 luồng hoạt động chuẩn | tr.9–26 là ảnh scan, không trích được text |
| `panel-e037a75-a0-spec.pdf` | Spec panel E037A75-A0 (20 trang), phát hành 2025-06-30 | Shenzhen Yingruida Optoelectronics (英瑞达) | tr.4–5 mô tả và cơ khí; tr.6–8 chân FPC; tr.9–10 mạch tham khảo; tr.11–12 điện và quang | Một số trang là hình |
| `module-schematic-4.2-3.7.pdf` | Sơ đồ nguyên lý bo module, v1.0, 2024-03-06 (1 trang, hình vector) | WeAct Studio | tr.1 | Dùng chung cho 4,2" và 3,7" |
| `module-board-shape-3.7.pdf` | Bản vẽ hình dạng và nhãn chân bo module (3 trang, nội dung ở tr.1) | WeAct Studio | tr.1 | |
| `ldo-me6231a33m3g-datasheet.pdf` | LDO 3,3 V trên module (11 trang) | Microne | tr.1–3 | |
| `connector-wafer-125w-8p-datasheet.pdf` | Đầu nối Wafer 1,25 mm (1 trang, hình) | Hanbo | tr.1 | |
| `panel-vendor-init-uc8253.c` | Mã mẫu hãng cho panel 3,7" đen trắng, MSP430 | Hãng panel | cả file | Mã hoá ký tự cũ (GBK): chú thích hãng bị lỗi khi mở bằng UTF-8, phần mã không ảnh hưởng |
| `example-arduino-esp32s3.ino` | Ví dụ Arduino của WeAct, dùng GxEPD2, có ghi chân ESP32-S3 | WeAct Studio | dòng 10–14, 29–30 | |

## Không copy vào repo

Thư mục gốc đầy đủ của WeAct nằm trên máy dev tại `E:\ESP32\Tài liệu cho ProjectIII\WeAct E-Paper Module 3.7 inch`. Những gì trong đó không nằm trong `source/`:

| Nội dung | Vị trí trong thư mục gốc | Lý do không copy |
|---|---|---|
| Mô hình 3D bo module 3,7" | `Hardware\WeAct-EpaperModule-3.7 Board 3D.step` (khoảng 3,5 MB) | Lớn, chỉ cần khi thiết kế vỏ |
| Datasheet controller của cỡ khác | `Doc\` (JD79660A, JD79667AA, SSD1680, SSD1681, SSD1683) | Không phải controller của module 3,7" |
| Tài liệu panel của cỡ khác | `Doc\1.54...`, `Doc\2.13...`, `Doc\2.9...`, `Doc\4.2...`, `Doc\old\` | Không áp dụng cho 3,7" |
| Ví dụ Arduino ESP32, ESP32-C3, ESP8266 | `Example\` | Cùng thư viện, khác chân; bản ESP32-S3 đã copy |
| Ví dụ Raspberry Pi bằng C | `Example\EpaperModuleTest_RaspberryPi\` | Chưa đọc; có thể chứa trình điều khiển tự viết |
| Bảng hình minh hoạ sản phẩm | `Images\1.png` … `6.png` | Ảnh quảng cáo |
| Hướng dẫn lấy mẫu bitmap | `Tools\EPD取模教程 From 中景园.zip` | Công cụ của bên thứ ba, chưa đọc |
| Sơ đồ và bản vẽ của cỡ khác | `Hardware\` (1,54", 2,13", 2,9", 4,2") | Không áp dụng |

Thông tin về thư viện GxEPD2 (`src/gdey/GxEPD2_370_GDEY037T03.cpp`, `src/GxEPD2_BW.h`, https://github.com/ZinggJM/GxEPD2, giấy phép GPL-3.0) không có trong repo này. Các thông số lấy từ đó ghi rõ "GxEPD2" ở cột nguồn và có trạng thái `Chưa xác nhận`, trừ khi đã chạy được trên máy.

## Khi không tìm ra thông tin

Thứ tự tra: file docs, rồi PDF trong `source/`, rồi thư mục gốc trên máy dev, rồi hỏi người làm. Không đoán thông số. Chi tiết ở `index.md`.
