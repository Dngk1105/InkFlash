# ADR 0004: Vòng đời app và task

- **Ngày:** 2026-10-23
- **Trạng thái:** Đã chốt

## Bối cảnh

Thiết bị sẽ có nhiều app (Flashcard, Settings, sau này Pomodoro, từ điển). ESP-IDF **không** có khái niệm app hay
tiến trình: cả firmware là một image, mọi task dùng chung heap, không có cách ly bộ nhớ.

ESP-IDF cung cấp các công cụ quản lý task của FreeRTOS: tạo task, priority, ghim core, suspend/resume, task
notification, Task Watchdog, đo stack (`uxTaskGetStackHighWaterMark`), liệt kê task (`vTaskList`), hàm dọn dẹp khi task
bị xoá (`vTaskSetThreadLocalStoragePointerAndDelCallback`).

Hai giới hạn quan trọng:
- Task Watchdog chỉ báo lỗi hoặc reset cả chip, **không huỷ được riêng task treo**.
- `vTaskDelete` một task từ bên ngoài **không** giải phóng bộ nhớ task đó cấp, không nhả mutex, không trả ngoại vi.

## Quyết định

- **Service** là task sống suốt từ lúc boot, không bị tạo/xoá khi đang chạy. Vòng đời: `init → run → suspend/resume`
  điều khiển bằng message (`ink_services_broadcast`).
- **App không phải task.** Mọi app chạy trong task `app_runner`. App manager gọi `on_start → on_event/on_draw → on_stop`.
  Chuyển app = `on_stop` app cũ, thu hồi rò rỉ, `on_start` app mới.
- **Bộ nhớ theo app:** `app_alloc()/app_free()` ghi lại mọi khối của app đang chạy; sau `on_stop` khối còn sót bị giải
  phóng và ghi log là rò rỉ. Bù cho việc không có cách ly bộ nhớ.
- **App cần chạy nền** thì nhờ service, không tự tạo task.
- **Không bao giờ xoá task từ bên ngoài.** Muốn dừng thì gửi lệnh, task tự dọn rồi tự xoá.
- **App treo:** `app_runner` đăng ký Task Watchdog → chip reset → (GĐ3) boot đếm crash liên tiếp, quá ngưỡng thì
  mở thẳng Settings (safe mode). ReviewLog đã nằm trên flash nên không mất tiến độ học.

## Để sau

- Nạp app động từ flash bằng component `elf_loader` (ESP Component Registry), như file `.fap` của Flipper. Chưa kiểm tra bản cho v6.1.
- Tách quyền user/kernel (ESP Privilege Separation): còn thử nghiệm.
