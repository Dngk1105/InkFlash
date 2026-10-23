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

## Kiến trúc firmware (v2, nhánh `arch-v2`)

Chi tiết: `docs/firmware/architecture.md`; lý do từng quyết định: `docs/adr/`.

- Luật phụ thuộc ép bằng CMake (`PRIV_REQUIRES ink_hal` trong services) và host test (domain/ui): vi phạm là lỗi build.
  Đã thử phá cả 4 luật ngày 2026-10-23.
- **Tầng dưới (drivers/) người dùng TỰ VIẾT và tối ưu.** Không tự viết driver thay người dùng trừ khi được yêu cầu.
  Hợp đồng driver: `components/drivers/README.md`. Code driver demo cũ ở tag `demo-v0` để tham khảo.
- Đặt tên component không trùng component của IDF (đã gặp: `hal`).
- Tham khảo: Flipper Zero (furi_hal, InputService, SceneManager), Watchy (deep sleep), InfiniTime, esp-bsp, Memfault SDK.

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

- Build: xoá `sdkconfig` khi đổi `sdkconfig.defaults`. Trình biên dịch đôi khi crash ngẫu nhiên (segfault ở esp_lcd): build lại là qua.
- Việc tiếp theo (GĐ1):
  1. Người dùng viết `drv_uc8253` và `drv_buttons_gpio` theo `components/drivers/README.md`, đăng ký trong `bsp_init()`.
  2. Mình làm font tiếng Việt cho `ui/`, dirty rect + chính sách làm mới + benchmark cho service render.
