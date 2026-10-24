# ESP32-S3-WROOM-1-N16R8: ràng buộc chân

- Không dùng GPIO22–25 (không tồn tại), GPIO26–32 (flash), GPIO33–37 (PSRAM octal).
- GPIO0, GPIO3, GPIO45, GPIO46 là chân strapping (chip đọc lúc khởi động để chọn chế độ). Dùng được nhưng phải chú ý mức kéo lên/xuống.
- GPIO19/20 là USB D-/D+ (console USB Serial/JTAG).
- Chân đánh thức deep sleep phải là RTC GPIO (GPIO0–21).
- Đo pin phải dùng ADC1 (GPIO1–10), vì ADC2 (GPIO11–20) tranh với WiFi.

## GPIO46

- Không phải RTC GPIO, nên không đánh thức được deep sleep.
- Là chân strapping, mặc định có pull-down yếu lúc reset. GPIO0 = 0 và GPIO46 = 0 thì vào download mode.
  GPIO46 cũng liên quan tới in ROM log.
- Nối nút có điện trở kéo lên ngoài vào GPIO46 (kéo lên 1 lúc reset) trong khi GPIO0 đang ở 0 thì chip không vào được
  download mode. Chỉ dùng điện trở kéo lên trong (bật bằng phần mềm) thì không bị.

Nguồn: esptool, mục Boot Mode Selection cho ESP32-S3.
