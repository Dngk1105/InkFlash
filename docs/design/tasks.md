# Chia task

## Vấn đề

Nhiều task cùng sửa một state thì sinh race condition: kết quả tuỳ task nào chạy trước. Mỗi task còn tốn một stack
trong RAM, và trước deep sleep phải chờ mọi task dừng. Máy chạy pin, ngủ là chính, nên càng ít task càng tốt.

## Cách xử lý

| Task | Việc | Vì sao tách |
|---|---|---|
| Main task | Giữ toàn bộ state: nhận message, nhận diện gesture, gọi app, ghi log, quyết định đi ngủ | Một nơi sửa state thì không cần lock |
| Display task | Owner duy nhất của panel: take frame, gọi driver, chờ panel | Panel bận 0,3–3 s mỗi lần vẽ |
| Helper task | Chỉ tạo khi có việc chặn lâu (ví dụ đẩy âm thanh liên tục, WiFi) | Không được chặn main task |

### Luật

- Event từ ISR và từ helper task đi vào main queue dưới dạng message. Helper task không sửa state của main task.
- Task do ESP-IDF tạo (task `esp_timer` chạy timer callback) cũng là helper task: timer callback chỉ post message vào
  main queue.
- Main task không chờ quá 50 ms cho một việc. Việc lâu hơn thì chuyển sang helper task.
- Muốn tách một module ra task riêng sau này thì module đó phải giao tiếp bằng message, không gọi hàm chéo.

### Vùng nhớ cho stack

ESP32-S3 có RAM nội và PSRAM (8 MB, nằm ngoài chip). Ràng buộc của ESP-IDF v6.1:

| Ràng buộc | Hệ quả |
|---|---|
| Lúc ghi flash, cache tắt nên PSRAM không đọc được | Task ghi file phải có stack trong RAM nội |
| Task có stack trong PSRAM không được gọi light sleep hay deep sleep | Main task có stack trong RAM nội |
| ISR không được đụng dữ liệu trong PSRAM | Dữ liệu ISR dùng nằm trong RAM nội |

Nguồn: `C:\esp\v6.1\esp-idf\docs\en\api-guides\external-ram.rst`, `docs\en\api-reference\system\sleep_modes.rst`.

## Tham khảo

- InkBoy (ESP32-S3, e-ink 30 fps): task chính làm menu và giữ màn, task riêng cho giả lập, âm thanh, ghi file.
- Flipper Zero chia mỗi service một thread; cách này hợp với Flipper vì chip STM32 ngủ mà vẫn giữ RAM. InkFlash deep
  sleep thì mất RAM.
