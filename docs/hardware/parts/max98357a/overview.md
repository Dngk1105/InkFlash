# MAX98357A

Mạch khuếch đại âm thanh số Class D mono, nhận tín hiệu qua giao tiếp I2S

| Thông số                          | Giá trị                             | Ghi chú                                                                   |
| ----------------------------------- | ------------------------------------- | -------------------------------------------------------------------------- |
| Điện áp cấp nguồn ($V_{DD}$) | 2,5 V – 5,5 V                        | Lấy trực tiếp từ đường pin/SYS ($3,0\text{ V} \sim 4,2\text{ V}$) |
| Công suất ra loa $8\ \Omega$    | 0,9 W (tại 3,7 V) / 1,8 W (tại 5 V) | Phù hợp loa $8\ \Omega$ 1 W của sản phẩm                            |
| Hiệu suất chuyển đổi           | 92%                                   | Chuẩn Class D không cần cuộn lọc ngoài (Filterless)                  |
| Dòng tắt nguồn (Shutdown)        | 0,6 µA                               | Khi chân `SD_MODE` ở mức 0                                            |
| Dòng tĩnh không tải             | 2,4 mA                                | Khi có clock I2S nhưng không có tín hiệu âm thanh                   |
| Tần số lấy mẫu ($f_S$)        | 8 kHz – 96 kHz                       | Tự nhận diện xung nhịp, không cần chân MCLK                         |
| Định dạng âm thanh              | I2S chuẩn (16, 24, 32 bit)           | 3 dây: BCLK, LRCLK, DIN                                                   |

## Cấu hình chân SD_MODE (Chọn kênh & Tắt nguồn)

Chân `SD_MODE` tích hợp hai chức năng: tắt IC và chọn kênh âm thanh để phát:

- **Mức thấp (< 0,16 V):** Tắt IC hoàn toàn (Shutdown, ăn dòng 0,6 µA).
- **Kênh Trái (Left Channel):** Kéo lên VDD bằng điện trở nhỏ ($100\text{ k}\Omega$) hoặc kéo thẳng GPIO mức cao.
- **Kênh Phải (Right Channel):** Kéo lên VDD bằng điện trở lớn ($300\text{ k}\Omega$).
- **Trộn kênh (Mono Mix Trái/2 + Phải/2):** Nối cầu phân áp điện trở giữa VDD và GND.

## Ràng buộc phần cứng

- **Nguồn cấp:** Lấy trực tiếp từ đường pin (`SYS`), không chạy từ đường 3,3 V qua LDO. Khi phát âm thanh lớn, dòng đỉnh có thể làm sụt áp rail 3,3 V của ESP32 nếu dùng chung.
- **Ngõ ra loa (OUTP và OUTN):** Là ngõ ra cầu vi sai BTL (Bridge-Tied Load). **Tuyệt đối không nối chung** hai chân này với nhau và **không nối bất kỳ chân nào xuống mass (GND)**.
- **Loa tương thích:** Loa thông dụng $8\ \Omega$ 1 W.
