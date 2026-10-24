# INMP441

Microphone MEMS đa hướng ngõ ra số I2S, cổng thu âm mặt đáy (Bottom Port)

| Thông số                            | Giá trị          | Ghi chú                                               |
| ------------------------------------- | ------------------ | ------------------------------------------------------ |
| Điện áp cấp nguồn ($V_{DD}$)   | 1,8 V – 3,3 V     | Cấp từ đường 3,3 V (sau công tắc tải TPS22916) |
| Dòng tiêu thụ hoạt động         | 1,4 mA             | Khi xung clock I2S đang chạy                         |
| Dòng tiêu thụ ở chế độ chờ    | 10 µA             | Tự động vào Standby khi ngắt xung SCK             |
| Độ nhạy (Sensitivity)              | -26 dBFS           | Tại 1 kHz, 94 dB SPL                                  |
| Tỉ số tín hiệu trên nhiễu (SNR) | 61 dBA             | Dải 20 Hz – 20 kHz, A-weighted                       |
| Dải tần số đáp ứng              | 60 Hz – 15.000 Hz | Đáp ứng phẳng                                      |
| Định dạng âm thanh                | I2S 24-bit         | Khung 32-bit slot                                      |
| Tần số lấy mẫu khuyến nghị      | 16 kHz             | Chuẩn tối ưu cho nhận diện giọng nói            |

## Ràng buộc phần cứng

- **Chân chọn kênh (`L/R`):** Tuyệt đối **không được để hở (float)**. Bắt buộc phải nối cố định:
  - Nối xuống **GND**: Mic xuất dữ liệu ở kênh Trái (Left Channel, khi xung WS ở mức thấp).
  - Nối lên **3,3 V**: Mic xuất dữ liệu ở kênh Phải (Right Channel, khi xung WS ở mức cao).
- **Lỗ thu âm (Bottom Port):** Lỗ thu âm nằm ở đáy vỏ chip. Trên bo mạch PCB bắt buộc phải khoan lỗ thông xuyên qua mạch ngay dưới vị trí mic để sóng âm truyền vào màng rung.
- **Tiết kiệm pin:** Khi không thu âm, chỉ cần ngừng cấp xung nhịp clock `SCK`, mic sẽ tự động chuyển sang chế độ Standby tiêu thụ 10 µA. Để ngắt hoàn toàn, dùng công tắc tải TPS22916.
