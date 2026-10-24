# INMP441 - Omnidirectional MEMS Microphone with I2S Digital Output

> **Manufacturer:** TDK InvenSense  
> **Target Role:** Thu âm thanh số, nhận diện giọng nói qua chuẩn giao tiếp I2S.  
> **Package:** 4.72mm × 3.76mm × 1.0mm Surface-Mount (Bottom Port)  
> **Trạng thái InkFlash:** Dùng trên breadboard thử nghiệm; mic bản thương phẩm chưa chốt.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp cấp nguồn** | $V_{DD}$ | **1.8 ~ 3.3** | V | Cấp từ đường 3.3V (sau công tắc tải TPS22916) |
| **Dòng tiêu thụ hoạt động** | $I_{DD}$ | **1.4** (Typ) | mA | Khi xung nhịp clock I2S đang chạy |
| **Dòng tiêu thụ ở chế độ chờ** | $I_{STBY}$ | **10** (Typ) | µA | Tự động vào Standby khi ngắt xung SCK |
| **Tỉ số tín hiệu trên nhiễu (SNR)**| SNR | **61** | dBA | Đo trong dải 20Hz - 20kHz, A-weighted |
| **Độ nhạy (Sensitivity)** | S | **-26** | dBFS | Tại $1\text{ kHz}, 94\text{ dB SPL}$ |
| **Dải đáp ứng tần số** | - | **60 ~ 15,000** | Hz | Đáp ứng tần số phẳng, âm thanh rõ ràng |
| **Định dạng dữ liệu số** | - | **24** | bit | Định dạng 24-bit I2S trong khung 32-bit slot |
| **Tần số lấy mẫu ($f_S$)** | $f_S$ | **7.8 ~ 50** | kHz | Thường chạy ở $16\text{ kHz}$ cho nhận diện giọng nói |

---

## 2. Sơ đồ chân & Ghép nối I2S

| Chân # | Tên chân | Loại | Kết nối ESP32-S3 | Chức năng chi tiết |
| :---: | :--- | :---: | :--- | :--- |
| **1** | `SCK` | Input | GPIO (I2S SCK) | Xung nhịp bit clock ($f_{SCK} = 64 \times f_S$). |
| **2** | `SD` | Output | GPIO (I2S DIN) | Tín hiệu dữ liệu âm thanh số nối tiếp 24-bit. |
| **3** | `WS` | Input | GPIO (I2S WS) | Xung chọn khung / chọn kênh ($f_{WS} = f_S$). |
| **4** | `L/R` | Input | GND hoặc 3.3V | **Chọn kênh:** Nối GND = Kênh Trái; Nối 3.3V = Kênh Phải. |
| **5, 6** | `GND` | Ground | GND | Mass chung hệ thống. |
| **7** | `VDD` | Power | 3.3V | Nguồn cấp (đặt tụ gốm $0.1\text{ µF}$ sát chân). |
| **8** | `CHIPEN` | Input | 3.3V / GPIO | Bật mic (HIGH = Hoạt động, LOW = Tắt hẳn). |

---

## 3. Ràng buộc phần cứng quan trọng
- **Chân `L/R`:** Tuyệt đối **KHÔNG ĐƯỢC ĐỂ HỞ (FLOAT)**. Bắt buộc phải hàn cố định xuống GND (chọn kênh Trái) hoặc lên 3.3V (chọn kênh Phải).
- **Lỗ thu âm (Acoustic Port):** Nằm ở mặt dưới vỏ chip (Bottom Port). Khi thiết kế PCB, phải có lỗ xuyên tâm qua bo mạch để sóng âm đi vào màng rung.
