# InkFlash — thiết bị học flashcard e-ink

## Người dùng

- Sinh viên, làm đề tài cá nhân. Kết hợp học thiết kế PCB và 3D
- Trả lời tiếng Việt, ngắn gọn, giải thích thuật ngữ ở lần đầu xuất hiện.

## Mục tiêu sản phẩm

Thiết bị flashcard bỏ túi, pin dùng nhiều tuần. Chạy đầy đủ khi offline. Khi có WiFi đã thiết lập
thì đồng bộ, chia sẻ deck và tích hợp thêm tính năng AI.

## Các quyết định đã chốt

- MCU: ESP32-S3-WROOM-1-N16R8 (anten PCB). ESP-IDF + FreeRTOS. Không dùng GPIO35–37 (PSRAM).
- Màn: module e-paper WeAct 3.7" = panel Good Display GDEY037T03, controller UC8253, 240×416, SPI.
  Theo thư viện GxEPD2 và demo chạy được; CHƯA đối chiếu datasheet. Chi tiết: `docs/hardware/display-weact-3.7.md`.
- 2 nút bấm, nối RTC GPIO, không có nút nguồn. PCB: nút A = GPIO0 (kiêm download mode).
  Breadboard GĐ0 (đấu 2026-10-21): nút A = GPIO8 (RTC, đánh thức được), nút B = GPIO46 (không phải RTC, không đánh thức
  được, chân strapping). Trên PCB cả hai nút bắt buộc là RTC GPIO. Pinmap đầy đủ: `docs/hardware/pinmap.md`.
- Breadboard GĐ0 dùng BNO055 làm IMU tạm (thay LSM6DSO/LIS2DH12 chưa có). Sản phẩm vẫn theo quyết định IMU ở dưới.
- Năng lượng: tự ngủ kiểu máy đọc sách, e-ink giữ màn nền (số thẻ cần ôn, pin). Bấm nút thì thức, về đúng thẻ;
  lần bấm đánh thức không tính là thao tác. A+B giữ 2s = ngủ, 8s = ship mode. Dòng ngủ < 20µA.
- IMU (LSM6DSO hoặc LIS2DH12): cử chỉ chỉ chạy khi thức. Không dùng IMU để đánh thức. Không dùng MPU6050.
- Audio: mic ICS-43434/INMP441 (I2S) + amp MAX98357A (chạy thẳng từ pin) + loa 8Ω 1W.
- AI trên máy: ESP-SR MultiNet7 (tiếng Anh), nạp từ của phiên ôn + từ dễ nhầm.
  Chấm phát âm chi tiết, TTS, LLM chạy trên server riêng (không để API key trên thiết bị).
- Nguồn: LiPo 503450 (~1000mAh), sạc BQ25180 (ship mode), LDO RT9080-33 (600mA, Iq ~2µA; TPS7A02 KHÔNG đủ dòng),
  công tắc tải TPS22916 cho khối phụ. microSD tuỳ chọn (SDMMC 1-bit).
- Lập lịch ôn: FSRS. ReviewLog chỉ ghi thêm (có CRC); nội dung thẻ: bản sửa sau thắng. Media lưu theo hash nội dung.

## Kiến trúc firmware

- 5 tầng: Cloud Server → Remote (WiFi/sync/OTA/hàng đợi offline)
  → Services (GUI, Audio, AI, Storage+SRS, Power) → Core (event bus, log, health, config) → HAL/driver + BSP.
- Mỗi service một task, chỉ giao tiếp qua event bus. Device registry: init/self_test/suspend/resume/recover.
- Chịu lỗi: thử lại → reset bus → bật/tắt nguồn → DEGRADED. Watchdog, core dump, safe mode khi crash liên tục,
  OTA rollback (cấm deep sleep đến khi firmware mới được xác nhận).
- Tách firmware/hardware (bắt buộc): service chỉ `REQUIRES` core, device_registry, ink_hal; không bao giờ bsp hay drv_*.
  Chân chỉ khai báo trong `components/bsp/include/boards/*.h`. Driver nhận chân qua struct cấu hình. Chi tiết: `docs/firmware/architecture.md`.
  Đã có HAL + driver (hiện là giả lập, trừ nút bấm): imu, display, mic, speaker, power. Linh kiện mới theo cùng khuôn `drv_imu`.
  Đặt tên component mới không trùng component của IDF (đã gặp: `hal`).
- Log: mỗi linh kiện một mã component, có event ID; ra console, RAM ring buffer, flash; console chẩn đoán `esp_console`.
- Phân vùng 16MB: offset 0x10000; nvs, nvs_keys, otadata, phy, coredump 64K, ota_0/ota_1 3M,
  model 4M (srmodels), storage ~5.7M (LittleFS).
- Tham khảo: InfiniTime, Flipper Zero firmware, esp-box/esp-bsp, Watchy, Memfault SDK.

## Tra tài liệu (ESP-IDF v6.1, cài ở `C:\esp\v6.1\esp-idf`)

Thứ tự ưu tiên khi cần API, khoá Kconfig, chân GPIO của ESP32-S3:

1. MCP `espressif-docs` (`search_espressif_sources`). Giới hạn 40 lần/giờ, chỉ có bản docs mới nhất.
2. Docs offline đúng phiên bản: `C:\esp\v6.1\esp-idf\docs\en\` (`.rst`) và `Kconfig` trong `C:\esp\v6.1\esp-idf\components\`.
3. Web, chỉ khi hai nguồn trên không có.

Thông số phần cứng của dự án (chip màn, pinmap, nguồn) nằm trong `docs/hardware/`. Chưa có trong docs thì hỏi người dùng, không đoán.
Build/flash: dùng MCP `esp-idf` (`build_project`, `flash_project`), cấu hình trong `.mcp.json`.

## Lộ trình phần cứng

- GĐ0: DevKitC-1 N16R8 + WeAct 3.7" + breakout trên breadboard.
- GĐ2: thiết kế PCB, song song vỏ in 3D. Vào giai đoạn đóng gói sản phẩm

## Trạng thái

- Xong: nghiên cứu, báo cáo phần cứng, sơ đồ kiến trúc, pinmap GPIO (breadboard + PCB).
- Xong (2026-10-21): khung project build được với v6.1. Có `sdkconfig.defaults` (16MB, PSRAM octal, rollback, coredump),
  `partitions.csv`, components `core`, `device_registry`, `bsp`, `services`, `remote`, console `status`/`log`.
  Cấu hình nằm trong `sdkconfig.defaults`; `sdkconfig` là file sinh ra.
- Xong (2026-10-21): driver thật cho màn UC8253, BNO055, INMP441, MAX98357A, WS2812 (đã build, CHƯA chạy trên máy)
  + `svc_demo` chuyển thể từ `E:\ESP32_projects\test\src\main.cpp`. Hướng dẫn chạy và checklist lần đầu: `docs/firmware/demo.md`.
  Mặc định breadboard nằm trong `sdkconfig.defaults` (sdkconfig sinh ra, xoá đi khi đổi mặc định).
  Phép xoay tọa độ và đóng gói bit của driver màn đã thử trên máy tính (host test, 4 hướng xoay).
- Việc tiếp theo:
  1. Flash và chạy demo trên breadboard, đối chiếu bảng "Lần chạy đầu tiên" trong `docs/firmware/demo.md`
  2. Đối chiếu chuỗi lệnh UC8253 với datasheet; viết driver nguồn thật khi có BQ25180/TPS22916
