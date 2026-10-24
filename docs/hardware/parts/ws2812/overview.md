# WS2812

LED RGB tích hợp IC điều khiển, giao tiếp nối tiếp 1 dây (Single-wire NZR)

| Thông số                          | Giá trị      | Ghi chú                                                  |
| ----------------------------------- | -------------- | --------------------------------------------------------- |
| Điện áp cấp nguồn ($V_{DD}$) | 3,5 V – 5,3 V | Hoạt động tốt ở mức 3,3 V hoặc 5 V                 |
| Dòng tiêu thụ cực đại         | ~60 mA         | Khi cả 3 màu R, G, B sáng 100% độ sáng              |
| Dòng tiêu thụ tĩnh              | ~1 mA          | Tiêu thụ nội tại của chip điều khiển khi LED tắt |
| Tần số truyền dữ liệu          | 800 kHz        | Chu kỳ bit $1,25\text{ µs}$                           |
| Định dạng màu                   | 24-bit GRB     | 8-bit Green, 8-bit Red, 8-bit Blue (MSB trước)          |

## Định thời xung truyền dữ liệu

|      Tín hiệu      | Thời gian chuẩn |  Dung sai  | Mô tả                              |
| :-------------------: | :---------------: | :--------: | :----------------------------------- |
| **$T_{0H}$** |     0,35 µs     | ±0,15 µs | Thời gian mức cao của Bit `0`   |
| **$T_{0L}$** |     0,80 µs     | ±0,15 µs | Thời gian mức thấp của Bit `0` |
| **$T_{1H}$** |     0,70 µs     | ±0,15 µs | Thời gian mức cao của Bit `1`   |
| **$T_{1L}$** |     0,60 µs     | ±0,15 µs | Thời gian mức thấp của Bit `1` |
| **$T_{RST}$** |     > 50 µs     |     -     | Xung chốt khung hình (Reset code)  |

## Điều khiển từ ESP32-S3

- Do yêu cầu độ chính xác định thời tính bằng nano-giây, không dùng vòng lặp delay phần mềm.
- **Giải pháp:** Sử dụng bộ ngoại vi **RMT (Remote Control)** tích hợp của ESP32-S3 qua thư viện `led_strip` của ESP-IDF, xuất chuỗi xung chính xác bằng phần cứng không tốn tài nguyên CPU.
- **Quản lý nguồn:** Khi LED tắt, dòng tĩnh ~1 mA vẫn tồn tại. Trong bản mạch thương phẩm có khối phụ nguồn, đường nguồn LED được cắt bằng công tắc tải TPS22916 khi vào Deep Sleep.
