# Chia task

## Vấn đề

Nhiều task cùng sửa một trạng thái thì sinh race condition (kết quả tuỳ task nào chạy trước). Mỗi task còn tốn một
stack trong RAM, và trước khi ngủ sâu phải chờ mọi task dừng. Máy chạy pin, ngủ là chính, nên càng ít task càng tốt.

## Cách xử lý


| Task            | Việc                                                                                                         | Vì sao tách                                      |
| ----------------- | --------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- |
| main            | Giữ toàn bộ trạng thái: nhận sự kiện, nhận diện cử chỉ, gọi app, ghi log, quyết định đi ngủ | Một nơi sửa trạng thái thì không cần khoá |
| màn            | Chủ duy nhất của màn: nhận khung, gọi driver, chờ panel                                                | Panel bận 0,3–3 s                                |
| Task phụ khác | Chỉ tạo khi có việc chặn lâu (ví dụ đẩy âm thanh liên tục, WiFi)                                 | Không được chặn task main                     |

### Luật

- Sự kiện từ ngắt và từ task phụ đi vào hàng đợi của task main. Task phụ không sửa trạng thái của task main.
- Task do ESP-IDF tạo (REPL của console, task chạy callback hẹn giờ của `esp_timer`) cũng là task phụ: lệnh console và callback hẹn giờ chỉ đẩy message vào hàng đợi của task main.
- Task main không chờ quá khoảng 50 ms cho một việc. Việc nào lâu hơn thì đẩy sang task phụ.
- Muốn tách một khối ra task riêng sau này thì khối đó phải giao tiếp bằng message, không gọi hàm chéo.

### Vùng nhớ cho stack

ESP32-S3 có RAM nội và PSRAM (8 MB, nằm ngoài chip). Ràng buộc của ESP-IDF v6.1:


| Ràng buộc                                                               | Hệ quả                                     |
| --------------------------------------------------------------------------- | ---------------------------------------------- |
| Lúc ghi flash, cache tắt nên PSRAM không đọc được                | Task ghi file phải có stack trong RAM nội |
| Task có stack trong PSRAM không được gọi light sleep hay deep sleep | Task main có stack trong RAM nội           |
| ISR không được đụng dữ liệu trong PSRAM                           | Dữ liệu ISR dùng nằm trong RAM nội      |

Nguồn: `C:\esp\v6.1\esp-idf\docs\en\api-guides\external-ram.rst`, `docs\en\api-reference\system\sleep_modes.rst`.

## Tham khảo

- InkBoy (ESP32-S3, e-ink 30 fps): task chính làm menu và giữ màn, task riêng cho giả lập, âm thanh, ghi file.
- Flipper Zero chia mỗi service một thread, hợp với nó vì chip STM32 ngủ mà vẫn giữ RAM. InkFlash ngủ sâu thì mất RAM.
