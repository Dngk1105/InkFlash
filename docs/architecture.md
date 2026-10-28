# Kiến trúc tổng thể

Máy ngủ sâu gần như cả ngày. Mỗi lần thức trên ESP32 là một lần khởi động lại từ đầu, nên firmware chạy theo
**phiên thức**: khởi động → khôi phục trạng thái → dùng → lưu → ngủ sâu.

## Các khối

| Khối | Việc | Task | C thuần | Được gọi |
|---|---|---|---|---|
| `apps/` | Flashcard, Cài đặt, menu: màn hình và luồng thao tác | main | | `system`, `study`, `display`, `ui` |
| `system/` | Lấy sự kiện từ hàng đợi, đưa cho `session` và app, chạy ngăn xếp scene, đi ngủ | main | | `session`, `gesture`, `keymap`, `scene`, `hal`, `platform` |
| `study/` | Hàng thẻ đến hạn, chấm điểm | main | Có | `fsrs`, `record`; storage và đồng hồ qua cổng |
| `display/` | Chủ duy nhất của màn: nhận khung, chọn cách vẽ, gọi driver, đo thời gian | màn | | `display_policy`, `hal`, `platform` |
| `display_policy/` | Phần quyết định của `display`: so khung, tìm vùng đổi, chọn partial/full, đếm bóng mờ | màn | Có | Chỉ thư viện C chuẩn |
| `session/` | Máy trạng thái phiên thức | main | Có | Chỉ thư viện C chuẩn |
| `gesture/` | Nút, IMU → cử chỉ | main | Có | Chỉ thư viện C chuẩn |
| `keymap/` | Cử chỉ → intent theo bảng gán của app; kiểm tra bảng gán | main | Có | `gesture` (chỉ dùng kiểu cử chỉ) |
| `scene/` | Ngăn xếp scene trong app | main | Có | Chỉ thư viện C chuẩn |
| `fsrs/`, `record/` | Tính lịch ôn, mã hoá bản ghi | main | Có | Chỉ thư viện C chuẩn |
| `ui/` | Canvas, font | main | Có | Chỉ thư viện C chuẩn |
| `storage/` | Đọc ghi file trên LittleFS | main | | `platform` |
| `platform/` | Log, config, giờ, hẹn giờ, console, RTC memory | nơi gọi | | ESP-IDF |
| `hal/` | Interface phần cứng | nơi gọi | | |
| `drivers/` | Mỗi chip một component | nơi gọi | | Làm theo interface `hal` |
| `bsp/` | Bảng chân, chọn driver theo board | lúc khởi động | | `hal`, `drivers` |

Khối không có trong cột "Được gọi" của một khối thì khối đó không được gọi tới.

## Task

| Task | Việc |
|---|---|
| main | Giữ toàn bộ trạng thái |
| màn | Chủ duy nhất của màn, chỉ vẽ khung mới nhất |
| Task phụ khác | Chỉ tạo cho việc chặn lâu |

## Hai luồng chính

**Lật thẻ:** ngắt nút → hàng đợi main → `gesture` ra cử chỉ → `keymap` ra intent → app đổi scene, vẽ canvas → giao khung cho task màn →
`display` chọn partial/full → driver vẽ.

**Ngủ và thức:** hết giờ chờ hoặc chọn Ngủ trong menu → `session` ra lệnh lưu → mỗi khối ghi snapshot vào ô RTC của mình →
vẽ màn nền → ngủ sâu. Bấm nút → khởi động → đọc snapshot → nạp màn nền làm ảnh cũ → vẽ lại đúng màn trước, không chớp.

## Gặp vấn đề thì đọc file nào

Mỗi file tự đứng được, chỉ mở file khớp với vấn đề.

| Gặp vấn đề | Đọc |
|---|---|
| Sản phẩm cần làm gì, tính năng có trong phạm vi không | [requirements.md](requirements.md) |
| Khối này có được gọi khối kia không; logic dính FreeRTOS nên khó test; muốn thêm biến toàn cục; một thông tin nằm hai nơi; định làm sẵn cho tính năng chưa có | [design/principles.md](design/principles.md) |
| Task main bận lâu, muốn tạo task mới, race condition, stack để RAM nội hay PSRAM | [design/tasks.md](design/tasks.md) |
| Nút, cử chỉ, A+B, nhấn kép, IMU, lần bấm đánh thức, gán cử chỉ → chức năng | [design/input.md](design/input.md) |
| Đi ngủ, thức dậy không về đúng chỗ, snapshot, RTC memory | [design/wake-session.md](design/wake-session.md) |
| Màn vẽ chậm, chớp, bóng mờ, viết hoặc sửa driver màn, đo tốc độ vẽ | [design/display-pipeline.md](design/display-pipeline.md) |
| Thêm app, scene, mục menu, BACK, Cài đặt | [design/app-framework.md](design/app-framework.md) |
| Mất dữ liệu khi mất điện, đổi định dạng file, nâng cấp dữ liệu, đồng bộ trùng hoặc thiếu | [design/data-and-sync.md](design/data-and-sync.md) |
| Thêm board, đổi chân GPIO, đổi linh kiện, muốn viết `#if BOARD` | [design/hardware-variants.md](design/hardware-variants.md) |
| Tạo khối mới, đặt tên, repo xếp thế nào, mã lỗi, template, `tools/check.py` báo lỗi | [design/code-structure.md](design/code-structure.md) |
| Vẽ chữ hoặc hình lên canvas, chữ Việt ra `?`, màn ngang bị xoay sai, thêm hoặc đổi font | [design/ui-canvas.md](design/ui-canvas.md) |
| Viết code gọi ESP-IDF, không chắc API còn dùng được ở v6.1, build báo deprecated | [design/esp-idf-api.md](design/esp-idf-api.md) |
| Thông số, chân, lệnh của một linh kiện; board dùng linh kiện nào, đấu chân nào | [hardware/index.md](hardware/index.md), rồi chỉ mở file cần |
| Viết hoặc sửa docs | [writing-docs.md](writing-docs.md) |
| Viết comment trong code, comment có cần không, comment lệch code | [writing-comments.md](writing-comments.md) |
| Làm visualize, file mô phỏng chạy trên trình duyệt, mô phỏng lệch code C | [writing-visualizations.md](writing-visualizations.md) |
