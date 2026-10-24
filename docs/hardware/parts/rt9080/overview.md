# RT9080-33

LDO 3,3 V cấp nguồn chính cho toàn bộ hệ thống

| Thông số                              | Giá trị      | Ghi chú                                                   |
| --------------------------------------- | -------------- | ---------------------------------------------------------- |
| Điện áp ra định danh ($V_{OUT}$) | 3,3 V          | Sai số ±2%                                               |
| Dải điện áp vào ($V_{IN}$)       | 1,6 V – 5,5 V | Phù hợp với dải áp pin LiPo (3,0 V – 4,2 V)          |
| Dòng tải tối đa ($I_{OUT}$)       | 600 mA         | Đáp ứng dòng đỉnh của ESP32-S3 khi phát sóng RF   |
| Dòng tĩnh không tải ($I_Q$)       | 2,0 µA        | Tiết kiệm pin tối đa khi vi điều khiển ngủ sâu    |
| Dòng khi tắt ($I_{SHDN}$)           | 0,1 µA        | Khi chân EN ở mức thấp                                 |
| Độ sụt áp (Dropout)                 | 310 mV         | Tại dòng cực đại 600 mA; chỉ 50 mV tại dòng 100 mA |
| Khử gợn nguồn (PSRR)                 | 75 dB          | Tại tần số 1 kHz                                        |
| Bảo vệ quá nhiệt ($T_{SD}$)       | 160 °C        | Tự phục hồi khi nhiệt độ hạ về 140 °C             |
| Giới hạn dòng bảo vệ               | 0,8 A – 1,2 A | Bảo vệ khi chập tải                                    |

## Ràng buộc phần cứng

- **Chân điều khiển (EN):** Mức logic cao ($V_{IH} \ge 1,1\text{ V}$), mức thấp ($V_{IL} \le 0,4\text{ V}$). Trên mạch sản phẩm, chân EN nối trực tiếp vào đường SYS để LDO luôn hoạt động khi máy bật.
- **Tụ lọc:** Bắt buộc dùng tụ gốm MLCC (X5R hoặc X7R) tối thiểu $2,2\text{ µF}$ ở cả đầu vào ($V_{IN}$) và đầu ra ($V_{OUT}$), đặt sát chân linh kiện.
- **Lý do chọn:** Không dùng TPS7A02 vì dòng tải tối đa của TPS7A02 chỉ đạt 200 mA, không đáp ứng được dòng đỉnh vọt lên 350–500 mA của ESP32-S3 khi phát WiFi.
