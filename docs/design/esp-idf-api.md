# Dùng đúng API ESP-IDF v6.1

## Vấn đề

Đảm bảo dùng đúng chuẩn API của ESP-IDF v6.1

## Cách xử lý

### API đúng 

Kiểm trong `C:\esp\v6.1\esp-idf`

| Việc                   | API cũ                             | Trạng thái trong v6.1                  | Dùng                                                    |
| ----------------------- | ----------------------------------- | ---------------------------------------- | -------------------------------------------------------- |
| I2C (IMU, chip sạc)    | `driver/i2c.h`                    | Còn, in cảnh báo lớn khi biên dịch | `driver/i2c_master.h`                                  |
| ADC (đo pin)           | `driver/adc.h`, `esp_adc_cal.h` | Đã xoá                                | `esp_adc/adc_oneshot.h`, `esp_adc/adc_cali_scheme.h` |
| I2S (loa, mic)          | `driver/i2s.h`                    | Đã xoá                                | `driver/i2s_std.h`                                     |
| Timer phần cứng       | `driver/timer.h`                  | Đã xoá                                | `driver/gptimer.h`                                     |
| Đánh thức bằng nút | `esp_sleep_enable_ext1_wakeup()`  | Còn, header ghi sắp bỏ                | `esp_sleep_enable_ext1_wakeup_io()`                    |
| Đọc nguyên nhân thức | `esp_sleep_get_wakeup_cause()` | Còn, đánh dấu deprecated | `esp_sleep_get_wakeup_causes()` (trả bitmap, kiểm bằng `BIT(ESP_SLEEP_WAKEUP_EXT1)`) |
| SPI (màn)              |                                     | Không đổi                             | `driver/spi_master.h`                                  |
| GPIO                    |                                     | Không đổi                             | `driver/gpio.h`, `driver/rtc_io.h`                   |

Ví dụ chính thức đúng bản nằm ở `C:\esp\v6.1\esp-idf\examples\` (ví dụ `system/deep_sleep`, `peripherals/i2c/i2c_basic`,
`peripherals/i2s/i2s_basic/i2s_std`, `peripherals/adc/oneshot_read`). Mỗi file mẫu ngoại vi ghi rõ ví dụ nó dựa theo.

Hướng dẫn chuyển đổi đúng bản nằm offline ở `C:\esp\v6.1\esp-idf\docs\en\migration-guides\`
(`release-5.x`, `release-6.x/6.0`, `release-6.x/6.1`). Đây là nguồn để lập danh sách API cấm.

### Ba lớp chặn API cũ

| Lớp                  | Cách làm                                                                                                                                                                                                                         | Bắt được                                                 |
| --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------ |
| 1. Template           | Mỗi ngoại vi dự án dùng có một ví dụ nhỏ build được bằng v6.1 trong `templates/` (GPIO + ngắt, SPI, I2C master, I2S, ADC, deep sleep, RTC memory, LittleFS, NVS). AI chép từ đây, không viết theo trí nhớ | AI không phải nhớ API                                     |
| 2. Trình biên dịch | Cờ `-Werror=deprecated-declarations` và `-Werror=cpp` trong CMake gốc: cảnh báo "deprecated" và `#warning` thành lỗi build                                                                                           | Mọi API v6.1 đánh dấu sắp bỏ, kể cả `driver/i2c.h` |
| 3. Script kiểm tra   | `tools/check.py` có danh sách cấm: header cũ, hàm cũ, API Arduino (`digitalRead`, `millis`, `delay`)                                                                                                                 | Thứ sót qua lớp 2, code chép từ ví dụ Arduino         |

Template build cùng repo. Khi nâng ESP-IDF lên bản sau, API nào bị đổi thì template build lỗi trước, chỉ ra chỗ
cần sửa trước khi code thật bị ảnh hưởng.

### Thứ tự tra cứu

| # | Nguồn                                                                                            | Lưu ý                                                                             |
| - | ------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------- |
| 1 | Template trong repo                                                                               | Đúng v6.1 và đúng quy ước dự án                                            |
| 2 | Docs offline `C:\esp\v6.1\esp-idf\docs\en\`, `Kconfig` trong `components\`, migration guide | Đúng bản                                                                         |
| 3 | MCP `espressif-docs`                                                                            | Chỉ có bản docs mới nhất, có thể khác v6.1; đối chiếu lại với nguồn 2 |
| 4 | Web                                                                                               | Cuối cùng; dễ gặp code cũ hoặc Arduino-ESP32                                  |
