# ESP32-S3-WROOM-1 - High-Performance 2.4GHz Wi-Fi & BLE 5.0 MCU Module

> **Manufacturer:** Espressif Systems  
> **Model:** ESP32-S3-WROOM-1-N16R8 (16MB Quad SPI Flash, 8MB Octal PSRAM)  
> **Package:** 41-pin Module with PCB Antenna  
> **Trạng thái InkFlash:** Vi điều khiển trung tâm (MCU) của toàn bộ dự án.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :--- |
| **Vi xử lý (CPU)** | Dual-core 32-bit Xtensa LX7 | - | Xung nhịp lên đến 240 MHz, tập lệnh AI Vector |
| **Bộ nhớ Flash ngoài** | **16** | MB | Kết nối qua chuẩn Quad SPI |
| **Bộ nhớ PSRAM ngoài** | **8** | MB | Chuẩn Octal SPI tốc độ cao (120 MHz) |
| **SRAM nội** | **512** KB SRAM + **16** KB RTC SRAM | - | Bộ nhớ trong chip |
| **Điện áp hoạt động** | **3.0 ~ 3.6** (Typ 3.3) | V | Yêu cầu nguồn ổn định, đáp ứng dòng đỉnh |
| **Dòng đỉnh phát RF (WiFi)** | **Up to 500** | mA | Đòi hỏi LDO RT9080 cấp dòng $\ge 600\text{ mA}$ |
| **Dòng ngủ sâu (Deep Sleep)** | **~8** | µA | Chỉ cấp điện cho RTC Timer và RTC GPIO |
| **Kết nối không dây** | Wi-Fi 802.11 b/g/n + Bluetooth 5.0 (LE) | - | Hỗ trợ BLE Mesh, 2 Mbps throughput |

---

## 2. Các chân cấu hình Boot (Strapping Pins)

> [!CAUTION]
> Các chân Strapping sau đây quyết định điện áp cấp và chế độ khởi động của chip. Không được đấu tải điện trở kéo ngoài làm sai lệch mức logic khi khởi động!

| Tên chân | GPIO | Mức logic khi Reset | Chức năng quyết định |
| :---: | :---: | :---: | :--- |
| **`BOOT`** | `GPIO0` | **1 (Cao):** Khởi động SPI Boot từ Flash<br>**0 (Thấp):** Vào chế độ nạp ROM Download (UART/USB) | Có trở kéo lên nội. Nối nút nhấn BOOT xuống GND. |
| **`VDD_SPI`** | `GPIO45` | **0 (Thấp):** Điện áp chân VDD_SPI là **3.3V** (Bắt buộc cho N16R8)<br>**1 (Cao):** Điện áp là 1.8V | **Tuyệt đối không kéo lên 1** lúc boot, sẽ làm hỏng/không đọc được Flash ngoài! |
| **`LOG`** | `GPIO46` | **0 (Thấp):** Cho phép xuất log ROM boot ra UART0<br>**1 (Cao):** Tắt log ROM boot | Kết hợp cùng GPIO0 trong quá trình khởi động. |
| **`JTAG`** | `GPIO3` | **0 (Thấp):** Kích hoạt USB-JTAG nội | Chân chọn cổng gỡ lỗi phần cứng. |

---

## 3. Ràng buộc các chân nối bộ nhớ Octal PSRAM
- Trên bản module **N16R8**, các chân sau đã được dùng để nối nội bộ với IC PSRAM 8-bit:
  - **`GPIO33`**, **`GPIO34`**, **`GPIO35`**, **`GPIO36`**, **`GPIO37`**
- **Quy tắc bất di bất dịch:** Tuyệt đối không câu dây, không kết nối hay cấu hình bất kỳ chân nào trong nhóm GPIO 33-37 ra mạch ngoài, nếu không chip sẽ gặp lỗi Panic/Crash ngay lập tức.
