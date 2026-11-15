# Thiết kế UI và viết app cho panel e-ink

## Vấn đề

Ví dụ: một màn menu thiết kế theo thói quen LCD có thanh chọn đen kín cả hàng, chạy lên xuống theo nút bấm, vẽ bằng
intent `UI`. Mỗi lần thanh chọn rời một mục rồi quay lại, mọi pixel của mục đó đổi màu hai lần và cộng debt 2 + 2.
Thanh chọn đi qua cùng một mục hai lượt là pixel ở đó chạm debt ceiling 8. Nếu người dùng đang bấm liên tục, display
lắc cả hàng 240 px đó trong 640 ms (cleanup fast); nếu không, cả màn chớp đen trắng khoảng 2,8 s (cleanup strong).
Mũi tên 16 × 16 px cũng tích debt như vậy, nhưng lần lắc chỉ chạm 256 pixel cạnh lề, gần như không ai thấy, và bóng mờ
nếu còn cũng chỉ nhỏ bằng mũi tên.

Panel e-ink không giống LCD ở ba điểm:

- Mỗi lần vẽ mất 240–340 ms, không phải 16 ms. Panel chỉ đổi khi được vẽ, và giữ hình khi mất nguồn.
- Mỗi lần đổi màu, pixel giữ lại một ít điện tích và màu chưa tới đích (xám, viền, bóng ảnh cũ). Display ghi phần nợ
  này thành debt theo từng pixel, rồi làm sạch bằng cleanup. Cleanup mạnh nhất chớp cả màn.
- Cách app vẽ quyết định mức debt và số lần chớp. Display không sửa được một thiết kế đổi quá nhiều pixel.

File này là luật cho người thiết kế màn và người viết app. Cơ chế bên dưới (intent, debt, cleanup, profile) nằm ở
`docs/design/display/display.md`; họa cụ vẽ nằm ở `docs/design/ui.md`; app, scene, command nằm ở
`docs/design/app-framework.md`. File này chỉ chép lại phần app cần biết.

## Cách xử lý

### Những gì app cần biết về display

| Khái niệm | Nghĩa với app | Số mặc định |
|---|---|---|
| Frame | Ảnh 1 bit cả màn, 240 × 416 theo chiều gốc của panel. Frame acquire được đã có sẵn ảnh của lần submit gần nhất | 3 frame trong RAM nội |
| Intent | App nói frame chứa gì; display chọn waveform theo đó | `AUTO`, `TEXT`, `UI`, `PAGE` |
| Flag | Yêu cầu thêm cho một frame | `DEFER_CLEANUP`, `REDRAW_WINDOW`; `BEFORE_SLEEP` chỉ `system` dùng |
| Debt | Mỗi pixel đổi màu cộng debt weight của intent | `TEXT` 1, `UI` 2, `PAGE` 1; tối đa 15 |
| Debt ceiling | Pixel sắp vượt ngưỡng này thì display làm sạch ngay trong lần vẽ đó | 8; trong active period tăng dần tới 15 |
| Active period | Frame tới trong 2 s sau frame trước. Lúc đó display ưu tiên cleanup fast thay cho cleanup strong | 2 s |
| Cleanup strong | Chớp cả màn, làm sạch mọi pixel, debt về 0 | 2,52 s (`tr4`, 20–30 °C) + 240 ms lượt đẩy |
| Cleanup fast | Chỉ lắc các pixel bẩn trong changed window, vẽ luôn frame mới | 640 ms (`clean_full50`) |
| Idle | Không có frame mới trong 20 s và còn debt: display tự chạy cleanup strong | `strong_ms` = 20 s |

Thời gian một lần vẽ theo intent:

| Intent | Profile | DRF | Hợp với |
|---|---|---|---|
| `UI` | `lut10f50` | 240 ms | Con trỏ, mục chọn, nút nhỏ |
| `TEXT` | `tr8` | 340 ms | Trang chữ để đọc |
| `PAGE` | `tr8`, cleanup strong nếu debt lớn nhất ≥ 6 | 340 ms hoặc khoảng 2,8 s | Sang scene khác |

Display tự làm mọi việc làm sạch. App không vẽ frame trắng rồi frame đen để "xoá bóng", và không tự hẹn giờ làm sạch.
Hai việc đó cộng debt và gây chớp thừa.

### Một vòng vẽ của app

Ví dụ: người dùng bấm nút lật thẻ ở app Flashcard.

1. App nhận command (không biết nút nào), đổi state của scene.
2. App gọi acquire. Frame đã có mặt trước của thẻ.
3. App bind canvas vào frame, tô trắng vùng chữ cũ, vẽ chữ mặt sau. Phần không đổi (viền thẻ, tiêu đề) giữ nguyên.
4. App submit với intent `TEXT`.

Luật:

- Vẽ lại đúng vùng đổi. Tô lại cả màn rồi vẽ lại y hệt cũng không sao, vì display so với shown frame và chỉ tính pixel
  đổi màu. Nhưng vẽ một pixel khác đi là cộng debt cho pixel đó.
- Không lo submit dồn dập. Người dùng bấm nhanh hơn panel thì frame pending cũ bị drop; chỉ frame mới nhất được vẽ.
- Submit frame giống hệt shown frame thì display không vẽ, không cộng debt.
- Một frame mang một intent. Frame có cả chữ và nút thì chọn intent của phần quan trọng hơn, hoặc tách hai frame.

### Chọn intent và flag

| Tình huống | Intent | Flag |
|---|---|---|
| Di chuyển con trỏ, đổi mục chọn trong menu | `UI` | |
| Lật thẻ, sang trang chữ trong cùng scene | `TEXT` | |
| Mở scene mới, quay lại scene trước (`BACK`) | `PAGE` | |
| Mở popup đè lên nội dung, và lúc đóng popup | `UI` hoặc `TEXT` | `REDRAW_WINDOW` |
| Chuỗi frame phải hiện liền mạch (đếm ngược, hiệu ứng nhiều bước) | Theo nội dung | `DEFER_CLEANUP` ở mọi frame của chuỗi, bỏ ở frame cuối |
| Không chắc | `AUTO` | |

`AUTO` thành `UI` khi changed window nhỏ hơn 10 % diện tích panel, còn lại thành `TEXT`.

**Vẽ hai nhịp.** Thẻ có viền và chữ: submit viền với `UI` (240 ms), rồi vẽ chữ vào cùng frame và submit với `TEXT`.
Người dùng thấy phản hồi sớm, chữ hiện rõ sau. Màn 4/4 của `ink_demo` làm đúng như vậy.

**`REDRAW_WINDOW`.** Ví dụ: popup nền đen mở đè lên trang chữ. Pixel chữ vốn đen dưới popup đi đen → đen, nên không
được kéo, và chữ cũ hiện mờ trong khối đen (đo trên BB0). Có flag, display lắc mọi pixel trong changed window bằng
cleanup fast (640 ms). Đặt flag ở cả frame mở và frame đóng popup. Không đặt flag cho thay đổi thường: mỗi frame có
flag là một lần cleanup fast, và sau 4 lần cleanup fast thì lần thứ 5 thành cleanup strong.

### Design system cho panel 1 bit

**Lưới.** Màn dọc 240 × 416. Đặt mép trái và mép phải của thành phần hay đổi (con trỏ, ô chọn, vùng chữ đổi) trên bội
của 8 px. Display căn changed window theo 8 px; thành phần lệch lưới kéo theo pixel thừa vào window.

**Màu.** Chỉ có đen và trắng. Xám là pattern Bayer 4 × 4 do canvas dựng từ paint; app viết màu mong muốn, không tự vẽ
chấm.

| Dùng | Nên | Tránh |
|---|---|---|
| Chữ | Đen trên trắng | Xám (chữ xám bị làm thành đen hoặc trắng theo ngưỡng 50 %) |
| Nền, khối lớn | Trắng; đen đặc khi cần nhấn | Xám 50 % diện rộng nằm trên khối đen. Đo trên BB0: dither gần bàn cờ ở các hàng quét trước làm khối đen ở hàng quét sau (phía cáp FPC) xám đi. Lượt đẩy sau cleanup strong sửa được lúc cleanup, các lần vẽ `TEXT`/`UI` không có lượt đẩy |
| Phân vùng | Đường viền 1–2 px, khoảng trắng | Sọc ngang 1 px diện rộng: cùng hiện tượng như dither 50 % |

**Thành phần đổi theo nút bấm.** Giữ số pixel đổi màu mỗi lần bấm nhỏ nhất có thể. Số pixel không làm debt ceiling tới
chậm hơn, vì debt tính theo từng pixel; nhưng nó quyết định vùng bị lắc khi cleanup fast và vùng còn bóng mờ.

| Thành phần | Nên | Tránh |
|---|---|---|
| Mục đang chọn | Mũi tên, chấm, hoặc viền đậm quanh mục | Đảo màu cả hàng (đen kín) |
| Tiến độ | Thanh mảnh, mỗi bước tô thêm một đoạn | Vẽ lại cả thanh mỗi bước |
| Số đếm | Vùng số cố định, chỉ đổi chữ số | Đổi bố cục khi số đổi độ dài |
| Popup | Viền và nền trắng, đặt cố định; dùng `REDRAW_WINDOW` | Popup nền đen lớn mở đóng liên tục |

**Bố cục ổn định giữa các scene.** Tiêu đề, chân trang và lề giữ cùng vị trí ở mọi scene. Khi đổi scene, phần đứng
yên không đổi màu, không cộng debt, và lần `PAGE` ít khi phải cleanup strong.

**Chữ.** Font hiện có là GNU Unifont 16 px, đủ dấu tiếng Việt. Dùng họa cụ chữ của `ui` để xuống dòng, cắt dấu ba chấm và
căn lề, không tự tính toạ độ từng chữ.

### Khi nào màn chớp

Người dùng chấp nhận chớp khi đang không nhìn kỹ, và khó chịu khi đang đọc. App thiết kế sao cho lần chớp rơi vào lúc
đổi scene hoặc lúc idle.

| Màn chớp khi | Ai gây | App làm gì |
|---|---|---|
| Idle 20 s mà còn debt | Display | Không làm gì: đây là cách làm sạch chính |
| `PAGE` khi debt lớn nhất ≥ 6 | Display | Dùng `PAGE` cho lần đổi scene, nơi chớp ít gây khó chịu |
| Pixel vượt debt ceiling ngoài active period | Display | Giảm số pixel đổi mỗi lần bấm (mục trên) |
| Lần cleanup fast thứ 5 kể từ cleanup strong gần nhất | Display | Không đặt `REDRAW_WINDOW` cho thay đổi thường |
| Trước khi ngủ, còn debt | `system` | Không làm gì |
| Cold boot, sau lỗi vẽ | Display | Không làm gì |

Debt ceiling là chỗ display được phép chớp cả màn để đảm bảo màn sạch. App không tắt được cơ chế này, chỉ hoãn cleanup
stage bằng `DEFER_CLEANUP`.

### Kiểm tra một màn trước khi xong

1. Vẽ màn trên board, bấm qua mọi thao tác chính 20 lần liên tục. Đếm số lần chớp cả màn.
2. Để yên 20 s cho cleanup strong chạy, rồi nhìn ô đen gần cáp FPC (cạnh dưới khi màn dọc): phải đen đều như ô đen ở
   phía trên.
3. Mở rồi đóng mọi popup 5 lần: không còn chữ mờ dưới vùng popup.
4. Chụp màn bằng `tools/vision_test/capture.py` khi cần so trước và sau một thay đổi.
