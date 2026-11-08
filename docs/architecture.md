# Kiến trúc tổng thể

Máy dùng deep sleep gần như cả ngày. Trên ESP32-S3, mỗi lần wake là một lần boot lại từ đầu, nên firmware chạy theo
**wake session**: boot → restore state → dùng → save → deep sleep.

## Các module

| Module | Việc | Task | Core module | Được gọi |
|---|---|---|---|---|
| `apps/` | Flashcard, Settings, menu: màn hình và luồng thao tác | main | | `system`, `study`, `display`, `ui` |
| `system/` | Nhận message từ main queue, chuyển cho `session` và app, chạy scene stack, đi ngủ | main | | `session`, `input`, `display`, `keymap`, `scene`, `platform` |
| `study/` | Hàng thẻ đến hạn, chấm điểm | main | Có | `fsrs`, `record`; storage và đồng hồ qua port |
| `display/` | Owner duy nhất của panel: take frame kèm intent, chọn preset và profile, cleanup khi idle, tắt analog power, gọi driver, đo thời gian | display | | `display_policy`, `frame_slots`, `hal`, `platform` |
| `display_policy/` | Phần quyết định của `display`: so frame, tìm changed window, giữ debt và edge flag của từng pixel, chọn decision, cleanup stage khi idle, dựng old plane | display | Có | Chỉ thư viện C chuẩn |
| `frame_slots/` | Owner của 3 frame của `display`: acquire, submit, take, shown frame, đếm frame bị drop | main, display (caller giữ lock) | Có | Chỉ thư viện C chuẩn |
| `session/` | State machine của wake session | main | Có | Chỉ thư viện C chuẩn |
| `input/` | Shell module của `gesture`: interrupt nút, timestamp, timer theo deadline của `gesture`, resync, nhận gesture config và wait mask | main | | `gesture`, `hal`, `platform` |
| `gesture/` | Edge của nút, IMU → gesture | main | Có | Chỉ thư viện C chuẩn |
| `keymap/` | Gesture → keymap intent theo keymap table của app; kiểm keymap table | main | Có | `gesture` (chỉ dùng kiểu gesture) |
| `scene/` | Scene stack trong app | main | Có | Chỉ thư viện C chuẩn |
| `fsrs/`, `record/` | Tính lịch ôn, mã hoá bản ghi | main | Có | Chỉ thư viện C chuẩn |
| `ui/` | Canvas, font | main | Có | Chỉ thư viện C chuẩn |
| `storage/` | Đọc ghi file trên LittleFS | main | | `platform` |
| `platform/` | Log, config, giờ, timer, RTC memory | caller | | ESP-IDF |
| `hal/` | Interface phần cứng | caller | | |
| `drivers/` | Mỗi chip một component; driver panel mang danh sách profile waveform do `tools/profile_gen.py` sinh | caller | | Làm theo interface `hal` |
| `bsp/` | Pin map, chọn driver theo board | boot | | `hal`, `drivers` |

Module không có trong cột "Được gọi" của một module thì module đó không được gọi tới.

## Task

| Task | Việc |
|---|---|
| Main task | Giữ toàn bộ state |
| Display task | Owner duy nhất của panel; chỉ vẽ frame mới nhất |
| Helper task | Chỉ tạo cho việc chặn lâu |

## Hai luồng chính

**Lật thẻ:** interrupt nút → main queue → `input` gọi `gesture` ra gesture → `keymap` ra keymap intent → app đổi
scene, vẽ canvas → submit frame cho display task → `display` chọn preset theo intent của frame → driver vẽ. Khi idle,
`display` chạy cleanup edge, cleanup light, rồi tắt analog power.

**Ngủ và wake:** hết giờ chờ hoặc chọn Ngủ trong menu → `session` ra lệnh save → mỗi module ghi snapshot vào RTC slot
của mình → vẽ background frame (còn debt thì cleanup strong luôn), tắt analog power → deep sleep. Bấm nút → boot → đọc
snapshot → `display` coi background frame là shown frame → vẽ cả panel ra đúng màn trước bằng preset của intent, không
chớp.

## Gặp vấn đề thì đọc file nào

Mỗi file tự đứng được; chỉ mở file khớp với vấn đề.

| Gặp vấn đề | Đọc |
|---|---|
| Sản phẩm cần làm gì, tính năng có trong phạm vi không | [requirements.md](requirements.md) |
| Module này có được gọi module kia không; logic dính FreeRTOS nên khó test; muốn thêm biến toàn cục; một thông tin nằm hai nơi; định làm sẵn cho tính năng chưa có | [design/principles.md](design/principles.md) |
| Main task bận lâu, muốn tạo task mới, race condition, stack để RAM nội hay PSRAM | [design/tasks.md](design/tasks.md) |
| Nút, gesture, A+B, double press, IMU, lần bấm đánh thức, gán gesture → chức năng | [design/input/input.md](design/input/input.md) |
| Đi ngủ, wake không về đúng chỗ, snapshot, RTC memory | [design/wake-session.md](design/wake-session.md) |
| Panel vẽ chậm, chớp, ghosting, viền xám, intent của app, cleanup khi idle, đổi waveform (profile, LUT), viết hoặc sửa driver panel, đo tốc độ vẽ | [design/display/display.md](design/display/display.md) |
| Thêm app, scene, mục menu, `BACK`, Settings | [design/app-framework.md](design/app-framework.md) |
| Mất dữ liệu khi mất điện, đổi định dạng file, nâng cấp dữ liệu, đồng bộ trùng hoặc thiếu | [design/data-and-sync.md](design/data-and-sync.md) |
| Thêm board, đổi chân GPIO, đổi linh kiện, muốn viết `#if BOARD` | [design/hardware-variants.md](design/hardware-variants.md) |
| Tạo module mới, đặt tên, repo xếp thế nào, mã lỗi, template, `tools/check.py` báo lỗi | [design/code-structure.md](design/code-structure.md) |
| Vẽ chữ hoặc hình lên canvas, chữ Việt ra `?`, màn ngang bị xoay sai, thêm hoặc đổi font | [design/ui-canvas.md](design/ui-canvas.md) |
| Viết code gọi ESP-IDF, không chắc API còn dùng được ở v6.1, build báo deprecated | [design/esp-idf-api.md](design/esp-idf-api.md) |
| Thông số, chân, lệnh của một linh kiện; board dùng linh kiện nào, đấu chân nào | [hardware/index.md](hardware/index.md), rồi chỉ mở file cần |
| Thuật ngữ, đổi tên identifier | [glossary.yaml](glossary.yaml) |
| Viết hoặc sửa docs | [writing-docs.md](writing-docs.md) |
| Viết comment trong code, comment có cần không, comment lệch code | [writing-comments.md](writing-comments.md) |
| Làm visualize, file mô phỏng chạy trên trình duyệt, mô phỏng lệch code C | [writing-visualizations.md](writing-visualizations.md) |
