# TPS22916

Công tắc tải cắt nguồn các khối phụ ngoại vi khi hệ thống chuyển sang chế độ ngủ

| Thông số                                | Giá trị            | Ghi chú                                                         |
| ----------------------------------------- | -------------------- | ---------------------------------------------------------------- |
| Dải điện áp hoạt động ($V_{IN}$) | 1,0 V – 5,5 V       | Dùng cho đường 3,3 V hoặc đường pin                      |
| Dòng tải liên tục tối đa            | 2,0 A                | Dư tải cho toàn bộ linh kiện ngoại vi kết hợp            |
| Trở kháng khi dẫn ($R_{ON}$)         | 60 mΩ               | Sụt áp chỉ 6 mV tại dòng 100 mA ($V_{IN} = 3,3\text{ V}$) |
| Dòng rò khi tắt (Off-state)            | ~10 nA               | Triệt tiêu hoàn toàn rò điện của các IC ngoại vi       |
| Dòng tĩnh khi bật (On-state)           | 0,5 µA              | Dòng tiêu thụ nội tại khi hoạt động                      |
| Điện trở xả tải nhanh (QOD)          | 150 Ω               | Kéo ngõ ra về 0 V khi tắt (bản TPS22916B/C)                 |
| Ngưỡng điều khiển chân ON           | $\ge 1,0\text{ V}$ | Tương thích trực tiếp với GPIO của ESP32-S3               |

## Vai trò trong InkFlash

- **Cắt nguồn khối phụ:** Các IC ngoại vi như khuếch đại loa MAX98357A, micro số INMP441 và LED WS2812 vẫn có dòng tĩnh từ vài chục đến hàng trăm micro-ampe dù ở chế độ chờ. Công tắc TPS22916 đặt tại đầu nguồn nhánh cấp điện cho các khối này.
- **Điều khiển:** ESP32-S3 dùng một chân GPIO điều khiển chân `ON`. Trước khi vào Deep Sleep, kéo GPIO này xuống mức 0 để cắt nguồn hoàn toàn.
- **Xả tải nhanh (Quick Output Discharge):** Khi công tắc ngắt, điện trở xả nội 150 Ω kéo điện áp nhánh phụ về 0 V ngay lập tức, ngăn ngừa tụ điện xả chậm làm linh kiện bị treo hoặc dòng rò ký sinh ngược qua các chân I/O (phantom power).
