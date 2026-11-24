# Kiến trúc tổng thể

Máy dùng deep sleep gần như cả ngày. Trên ESP32-S3, mỗi lần wake là một lần boot lại từ đầu, nên firmware chạy theo
**wake session**: boot → restore state → dùng → save → deep sleep.

## Các module

| Module | Việc | Task | Core module | Có code | Được gọi |
|---|---|---|---|---|---|
| `apps/` | Home, Settings và các app khác: hàm vòng đời, app table, scene | main | | Có (Home) | `system`, `study`, `display`, `ui` |
| `system/` | Nhận message từ main queue, chuyển cho `session` và app, chạy scene stack, đi ngủ | main | | Có | `session`, `input`, `display`, `keymap`, `scene`, `ui` (vẽ background frame), `platform`; app qua con trỏ hàm do `main` đưa vào |
| `study/` | Hàng thẻ đến hạn, chấm điểm | main | Có | Chưa | `fsrs`, `record`; storage và đồng hồ qua port |
| `display/` | Owner duy nhất của panel: take frame kèm intent, chọn preset và profile, cleanup khi idle, tắt analog power, gọi driver, đo thời gian | display | | Có | `display_policy`, `frame_slots`, `hal`, `platform` |
| `display_policy/` | Phần quyết định của `display`: so frame, tìm changed window, giữ debt và edge flag của từng pixel, chọn decision, cleanup stage khi idle, dựng old plane | display | Có | Có | Chỉ thư viện C chuẩn |
| `frame_slots/` | Owner của 3 frame của `display`: acquire, submit, take, shown frame, đếm frame bị drop | main, display (caller giữ lock) | Có | Có | Chỉ thư viện C chuẩn |
| `session/` | State machine của wake session, kiểm header của RTC slot | main | Có | Có | Chỉ thư viện C chuẩn |
| `input/` | Shell module của `gesture`: interrupt nút, timestamp, timer theo deadline của `gesture`, resync, nhận gesture config và wait mask | main | | Có | `gesture`, `hal`, `platform` |
| `gesture/` | Edge của nút, IMU → gesture | main | Có | Có | Chỉ thư viện C chuẩn |
| `keymap/` | Gesture → command theo keymap table của app; kiểm keymap table | main | Có | Có | `gesture` (chỉ dùng kiểu gesture) |
| `scene/` | Scene stack trong app | main | Có | Có | Chỉ thư viện C chuẩn |
| `fsrs/`, `record/` | Tính lịch ôn, mã hoá bản ghi | main | Có | Chưa | Chỉ thư viện C chuẩn |
| `ui/` | Họa cụ để app vẽ lên frame: canvas, primitive, paint, chữ, thước | main | Có | Có | `u8g2`; `display_policy` (chỉ dùng kiểu frame info) |
| `u8g2/` | Component bọc thư viện U8g2 trong `third_party/`: chọn file biên dịch, dựng màn ảo trỏ vào frame | caller | | Có | U8g2 |
| `storage/` | Đọc ghi file trên LittleFS | main | | Chưa | `platform` |
| `platform/` | Log, config, giờ, timer, RTC slot, reset reason, deep sleep | caller | | Có | ESP-IDF |
| `hal/` | Interface phần cứng | caller | | Có | |
| `drivers/` | Mỗi chip một component; driver panel mang danh sách profile waveform do `tools/profile_gen.py` sinh | caller | | Có | Làm theo interface `hal` |
| `bsp/` | Pin map, chọn driver theo board | boot | | Có | `hal`, `drivers` |

Module không có trong cột "Được gọi" của một module thì module đó không được gọi tới. Cột "Có code" = "Chưa": kiến
trúc đã chốt, chưa có component trong `components/`. `system/` có code gồm main queue, main task, khởi động
`input` và `display`, mở app, chạy scene stack và chuyển command; phần đi ngủ chờ `session`.

## Task

| Task | Việc |
|---|---|
| Main task | Giữ toàn bộ state |
| Display task | Owner duy nhất của panel; chỉ vẽ frame mới nhất |
| Helper task | Chỉ tạo cho việc chặn lâu |

## Hai luồng chính

**Lật thẻ:** interrupt nút → main queue → `input` gọi `gesture` ra gesture → `keymap` ra command → app đổi
scene, vẽ canvas → submit frame cho display task → `display` chọn preset theo intent của frame → driver vẽ. Khi idle,
`display` chạy cleanup edge, cleanup light, rồi tắt analog power.

**Ngủ và wake:** hết giờ chờ hoặc chọn Ngủ trong Home → `session` ra lệnh save → mỗi module ghi snapshot vào RTC slot
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
| Panel vẽ chậm, chớp, ghosting, viền xám, intent của app, hợp đồng giữa app và display, đổi panel hoặc controller, cleanup khi idle, đổi waveform (profile, LUT), viết hoặc sửa driver panel, đo tốc độ vẽ | [design/display/display.md](design/display/display.md) |
| App vẽ chữ, hình, bố cục lên frame; chữ không xuống dòng; đổi font; vẽ sai chiều; đổi thư viện vẽ; thêm pixel format | [design/ui.md](design/ui.md) |
| Thêm app, scene, Home, `BACK`, Settings | [design/app-framework.md](design/app-framework.md) |
| Chuyển màn từ Figma sang code, view sinh ra, tên layer, luật kiểm thiết kế | [design/figma-to-code.md](design/figma-to-code.md) |
| Cấu hình Figma cho pixel: font, grid, màu xám, dither, icon | [design/figma-setup.md](design/figma-setup.md) |
| Thiết kế màn, bố cục, chọn intent và flag khi viết app, màn chớp hoặc còn bóng mờ vì cách app vẽ | [design/app-ui-guide.md](design/app-ui-guide.md) |
| Mất dữ liệu khi mất điện, đổi định dạng file, nâng cấp dữ liệu, đồng bộ trùng hoặc thiếu | [design/data-and-sync.md](design/data-and-sync.md) |
| Thêm board, đổi chân GPIO, đổi linh kiện, muốn viết `#if BOARD` | [design/hardware-variants.md](design/hardware-variants.md) |
| Tạo module mới, đặt tên, repo xếp thế nào, mã lỗi, template, `tools/check.py` báo lỗi | [design/code-structure.md](design/code-structure.md) |
| Viết code gọi ESP-IDF, không chắc API còn dùng được ở v6.1, build báo deprecated | [design/esp-idf-api.md](design/esp-idf-api.md) |
| Thông số, chân, lệnh của một linh kiện; board dùng linh kiện nào, đấu chân nào | [hardware/index.md](hardware/index.md), rồi chỉ mở file cần |
| Thuật ngữ, đổi tên identifier | [glossary.yaml](glossary.yaml) |
| Viết hoặc sửa docs | [writing-docs.md](writing-docs.md) |
| Viết comment trong code, comment có cần không, comment lệch code | [writing-comments.md](writing-comments.md) |
| Làm visualize, file mô phỏng chạy trên trình duyệt, mô phỏng lệch code C | [writing-visualizations.md](writing-visualizations.md) |
