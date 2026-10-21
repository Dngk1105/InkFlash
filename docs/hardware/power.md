# Nguồn và quản lý năng lượng

## Khối nguồn đã chọn

| Khối | Linh kiện | Thông số chính | Nguồn | Trạng thái |
|---|---|---|---|---|
| Pin | LiPo 503450 | ~1000mAh, 3.7V | | Đã chốt |
| Sạc | BQ25180 | I2C, có ship mode | | Đã chốt |
| LDO 3.3V | RT9080-33 | 600mA, Iq ~2µA | | Đã chốt |
| Công tắc tải | TPS22916 | Cấp nguồn cho khối phụ | | Đã chốt |

TPS7A02 không dùng được vì dòng ra không đủ cho ESP32-S3 khi bật WiFi.

## Ngân sách dòng ngủ (mục tiêu < 20µA)

| Thành phần | Dòng ngủ | Nguồn | Trạng thái |
|---|---|---|---|
| ESP32-S3 deep sleep (RTC GPIO wake) | | | |
| RT9080 Iq | ~2µA | | |
| BQ25180 (pin cấp, không sạc) | | | |
| E-paper deep sleep | | | |
| Khối phụ sau TPS22916 (đã tắt) | | | |
| **Tổng** | | | |

## Hành vi nguồn

- A+B giữ 2s: ngủ. A+B giữ 8s: ship mode (BQ25180 ngắt pin, cắm sạc để thoát).
- Trước khi ngủ: e-ink vẽ màn nền (số thẻ cần ôn, % pin), tắt TPS22916.
- Chưa xác nhận firmware OTA mới thì không vào deep sleep.
