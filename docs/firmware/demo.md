# Demo benchmark e-paper + IMU + loa

Demo chuyển thể từ `E:\ESP32_projects\test\src\main.cpp` (PlatformIO + Arduino) sang kiến trúc InkFlash.
Code lưu trữ tại `examples/demo_benchmark/svc_demo.c`, chỉ dùng HAL nên không biết chip nào đang chạy.
Mặc định dự án đã tắt demo để chuẩn bị lập trình sản phẩm theo từng module.

## Cách chạy

1. Cắm cổng USB-C "native" của DevKit (cổng nối GPIO19/20) vào máy tính.
2. Build, nạp và mở monitor bằng ESP-IDF v6.1:

   ```bash
   idf.py -p COMx flash monitor
   ```

   Thay `COMx` bằng cổng của bạn. Thoát monitor: `Ctrl+]`.
3. Màn hình tự làm mới toàn phần lần đầu (khoảng 3 giây vì có thêm một lần xoá trắng), LED chuyển cam → xanh lá, loa kêu một tiếng bíp.

## Điều khiển

| Thao tác | Việc xảy ra | Hàm gốc trong main.cpp |
|---|---|---|
| Bấm nút A (GPIO8) | 1 lần làm mới cục bộ, bíp 1000 Hz | `runPartialUpdate` |
| Bấm nút B (GPIO46) | Làm mới toàn phần | `runFullRefresh` |
| Gõ `1` + Enter ở console `ink>` | Đo 1 lần làm mới toàn phần | phím `1` |
| Gõ `2` + Enter | Chuỗi 10 lần làm mới cục bộ, in thời gian trung bình và FPS | `runPartialBenchmarkSequence` |
| Gõ `3` + Enter | Bật/tắt live tracker: cập nhật góc nghiêng mỗi 400 ms, cứ 25 frame làm mới toàn phần một lần | phím `3` |
| Gõ `4` + Enter | Deep clean: đảo cả màn đen rồi trắng, sau đó mới vẽ. Sạch hẳn bóng mờ và chấm lốm đốm, khoảng 8,3 s | không có |
| Gõ `hw` | Xem phần cứng nhìn qua HAL | không có |
| Gõ `btn` | Chẩn đoán nút: mức điện thật, số lần ngắt, số event | không có |

LED: cam = đang làm mới toàn phần, xanh dương = đang làm mới cục bộ, xanh lá = xong.

## Khác với bản Arduino

- Chữ dùng font 5×7 phóng to (`ink_gfx`) thay cho FreeMono, nên kiểu chữ khác.
- Nút B làm mới toàn phần, giống bản gốc. Trên breadboard nút B ở GPIO46 không đánh thức được từ ngủ sâu (xem `pinmap.md`).
- Trên màn, thời gian "FULL/PARTIAL" là số đo thật của lần trước. Bản gốc ghi cố định "~2.05 s / ~0.35 s".
- Phím `1/2/3` gõ ở console rồi Enter, vì console dùng chế độ theo dòng.
- Nhãn trạng thái dài tối đa 30 ký tự, để không tràn ra ngoài vùng làm mới cục bộ.

## Lần chạy đầu tiên: những gì cần kiểm tra

Code này **đã build được nhưng chưa chạy trên phần cứng**. Chỉ phần xoay tọa độ và đóng gói bit của màn, cùng bố cục chữ, đã được thử trên máy tính. Nếu có gì lạ, báo kèm log:

| Hiện tượng | Nghi ngờ |
|---|---|
| Màn không đổi, log báo `BUSY timeout` | Chân BUSY/RES sai, hoặc nguồn 3V3 yếu; chuỗi lệnh UC8253 |
| Hình bị xoay hoặc lật ngược | `BSP_EPD_ROTATION` trong `boards/breadboard_gd0.h` (0–3) |
| Log không thấy `BNO055 tai 0x..` | Dây I2C, chân ADDR/COM3, thiếu điện trở kéo lên; kiểm tra bằng `hw` |
| Loa im lặng hoặc rè | Dây I2S, nguồn loa 3,3 V yếu, âm lượng 25% mặc định |
| Loa kêu liên tục sau bíp | Bộ đệm I2S lặp lại; đã đặt `auto_clear_after_cb` nên nếu gặp thì báo |
| LED không sáng | GPIO48 là WS2812 của DevKit; bản DevKit khác có thể dùng chân khác |
| Nút B (GPIO46) không ăn | Chiều đấu nút (`BSP_BTN_ACTIVE_LOW`), chân strapping |
