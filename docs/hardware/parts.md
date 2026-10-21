# Linh kiện phụ

| Khối | Linh kiện | Giao tiếp | Địa chỉ / chân | Nguồn | Trạng thái |
|---|---|---|---|---|---|
| IMU (sản phẩm) | LSM6DSO hoặc LIS2DH12 | I2C | | | Chưa chọn 1 trong 2 |
| IMU (breadboard GĐ0) | BNO055 | I2C, SDA=GPIO17, SCL=GPIO18 | 0x28 hoặc 0x29 (tuỳ chân COM3) | Đã đấu dây | Dùng tạm; không nằm trong danh sách chip của sản phẩm |
| Mic | ICS-43434 hoặc INMP441 | I2S | | | Chưa chọn 1 trong 2 |
| Mic (breadboard GĐ0) | INMP441 | I2S, SCK=GPIO9, WS=GPIO10, SD=GPIO11 | | Đã đấu dây | Chân L/R phải nối GND hoặc 3V3, đừng để hở |
| Amp | MAX98357A | I2S, DOUT=GPIO12 | | Đã đấu dây (breadboard) | Đã chốt |
| LED trạng thái (breadboard) | WS2812 trên DevKit | 1 dây, GPIO48 | | Có sẵn trên DevKit | Chỉ dùng cho demo |
| Loa | 8Ω 1W | | | | Đã chốt |
| microSD | thẻ nhớ | SDMMC 1-bit | | | Tuỳ chọn |

## Ghi chú

- IMU chỉ dùng khi máy thức (tap, úp mặt để ngủ, xoay màn). Không dùng IMU để đánh thức. Không dùng MPU6050.
- Amp MAX98357A lấy nguồn thẳng từ pin, không cần mạch boost.
