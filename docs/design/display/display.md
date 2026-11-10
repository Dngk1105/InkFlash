# Đường vẽ màn

## Vấn đề

Lật thẻ phải nhanh (≤ 0,3 s, rồi giảm dần), chữ phải rõ, và panel không được hỏng dần vì ghosting hay lệch điện tích.
Gần hết thời gian vẽ là lúc panel quét, nằm trong driver. Waveform nạp qua thanh ghi của controller UC8253 cho DRF
31–343 ms. Waveform càng ngắn thì càng dễ để lại màu xám, viền hoặc bóng ảnh cũ. Không waveform nào vừa nhanh vừa
sạch, nên display chọn waveform theo nội dung đang vẽ và chạy cleanup lúc người dùng không để ý.

App không biết panel vẽ thế nào; app chỉ biết mình đang vẽ gì (một trang chữ, một con trỏ menu, một màn mới). Display
cần đúng thông tin đó (intent) để chọn waveform.

Controller có hai vùng RAM ảnh, ghi bằng lệnh `0x10` và `0x13`. Sau mỗi DRF, hai vùng hoán đổi vai "ảnh mới" và "ảnh cũ".
Driver chỉ ghi một vùng trước DRF thì mọi DRF thứ chẵn kể từ reset không đổi hình.

## Sơ đồ

Các sơ đồ nằm cùng thư mục, vẽ từ file này, mở bằng trình duyệt:

| File | Cho thấy |
|---|---|
| `architecture.html` | Các khối của đường vẽ màn và ai gọi ai |
| `sequence.html` | Một lần lật thẻ rồi các cleanup stage khi idle: submit frame kèm intent, chọn preset, driver ghi hai vùng RAM theo lượt, DRF, cleanup edge, tắt analog power |
| `dataflow.html` | Dữ liệu đổi dạng qua từng tầng: frame 1 bit và intent → pixel_state → window và hai plane → profile → RAM → cặp bit chọn waveform → điện áp |
| `frame-lifecycle.html` | Một frame đổi owner thế nào giữa main task, pending và display task, và khi nào frame bị drop |
| `draw-decision.html` | Thứ tự điều kiện để chọn decision cho một frame: cleanup strong, không vẽ, cleanup fast, hay vẽ theo preset |
| `idle-cleanup.html` | Các cleanup stage khi idle, flag `DEFER_CLEANUP` và tắt analog power |
| `hal-contract.html` | Các kiểu dữ liệu ở ranh giới giữa display và driver |
| `draw-timing.html` | Thời gian một lần đổi cả trang khi analog power đang tắt, ghép từ các số đo và số tính riêng lẻ |

Đổi đường vẽ màn thì vẽ lại các sơ đồ liên quan trong cùng commit.

## Một lần vẽ đi qua đâu

| # | Đoạn | Ai làm | Task |
|---|---|---|---|
| 1 | Bấm nút → gesture → lệnh của keymap | `gesture`, `keymap` | main |
| 2 | App vẽ vào frame (ảnh 1 bit cả màn) | app + `ui` | main |
| 3 | Submit frame kèm intent cho display task (3 frame trong RAM nội) | `display` | main → display |
| 4 | So với shown frame, tìm changed window, cập nhật pixel_state, chọn preset và old plane | `display` | display |
| 5 | Gửi profile (nếu khác lần trước), new plane và old plane của window qua SPI | driver | display |
| 6 | Panel quét; driver chờ chân BUSY | driver | display |

Người dùng bấm nhanh hơn tốc độ panel thì các frame pending ở giữa bị drop; chỉ frame mới nhất được vẽ. Giữa hai lần
bấm, display task chạy các cleanup stage (mục "Cleanup khi idle").

## Chuyển frame giữa main task và display task

Ví dụ: app lật thẻ. Main task acquire một frame. Frame này đã có sẵn ảnh của lần submit gần nhất, nên app chỉ vẽ phần
đổi. Vẽ xong, main task submit frame kèm `input_ms`, intent `TEXT` và flag. Display task thức dậy, take frame đó để vẽ.

Có 3 frame trong RAM nội. Mỗi frame luôn có đúng một owner:

| Owner | Frame |
|---|---|
| Main task | Frame đang vẽ (đã acquire, chưa submit) |
| Pending | Frame mới nhất đã submit, display task chưa take |
| Display task | Shown frame, và frame đang được vẽ ra panel |

- Display task giữ shown frame, vì driver cần nó làm old plane ở mỗi lần vẽ. Vẽ xong, frame vừa vẽ thành shown frame
  và shown frame cũ được free.
- Display task đang giữ hai frame mà main task acquire: main task lấy lại frame pending; frame đó tính là drop.
- Submit khi đã có frame pending: frame pending cũ bị drop và được đếm. Intent và flag đi theo frame mới.
- Acquire lần hai khi frame đã acquire chưa submit là lỗi của caller.

Việc đổi owner nằm trong khối C thuần `frame_slots`, có host test. Khối này chỉ giữ owner của từng frame, không giữ
bộ nhớ ảnh. Display gọi nó trong lock, rồi chép ảnh vào frame vừa acquire ở ngoài lock. Đọc ngoài lock vẫn an toàn,
vì frame submit gần nhất chỉ được free khi đã có frame mới hơn.

## Intent

Mỗi frame mang đúng một intent. Một DRF chỉ chạy một bộ waveform trên một window, nên frame có cả chữ lẫn nút thì app
chọn intent của phần quan trọng hơn.

| Intent | App dùng khi | Display ưu tiên |
|---|---|---|
| `AUTO` | Không muốn chọn | Window nhỏ hơn `auto_ui_percent` % diện tích panel thì là `UI`, còn lại là `TEXT` |
| `TEXT` | Trang chữ người dùng sẽ đọc (mặt thẻ) | Chữ rõ; 0,3–0,4 s chấp nhận được |
| `UI` | Con trỏ, mục chọn, nút: thành phần nhỏ người dùng không nhìn kỹ | Nhanh; chịu xám nhẹ, các cleanup stage làm sạch dần |
| `PAGE` | Đổi cả màn (sang scene khác) | Như `TEXT`; debt đã cao thì cleanup strong ngay trong lần vẽ này |

Flag đi kèm frame:

| Flag | Nghĩa |
|---|---|
| `DEFER_CLEANUP` | Không chạy cleanup stage nào cho tới frame sau không có flag này. Debt ceiling vẫn áp dụng; tắt analog power vẫn chạy |
| `BEFORE_SLEEP` | Frame là background frame. Còn debt thì vẽ bằng cleanup strong; vẽ xong thì tắt analog power |
| `REDRAW_WINDOW` | App vừa mở hoặc đóng một lớp đè lên nội dung. Display kéo mọi pixel trong changed window bằng cleanup fast, không chỉ pixel đổi màu |

Ví dụ cho `REDRAW_WINDOW`: popup đen đè lên trang chữ. App đặt flag ở frame mở popup (khối đen che chữ) và ở frame đóng
popup (trang chữ hiện lại). Không có flag, pixel chữ vốn đã đen mà popup cũng đen thì đi đen → đen. Pixel không đổi màu
chạy LUT giữ màu KK, mà KK của `lut10f50` là 0 V, nên mực cũ không bị kéo và chữ cũ hiện nhạt trong khối đen (đo trên BB0,
để yên vài giờ thì tự mất). Có flag, cleanup fast lắc cả pixel đen → đen, chữ cũ biến mất.

## Một lần gọi driver

Display không phân biệt full và partial. Mỗi lần gọi driver gồm bốn thứ; "full" chỉ là một cleanup có window cả panel
và old plane đảo bit:

| Thứ | Nghĩa |
|---|---|
| Window | Hình chữ nhật cần vẽ; mép x căn theo `x_align` của driver |
| Old plane | Bit cũ của từng pixel, chọn waveform của pixel đó (mục "Chia việc giữa display và driver") |
| Profile | Waveform, điện áp, LUT frame rate |
| Draw config | Có vào partial mode (PTL) không, `scan_outside_window` (bit PT_SCAN), có drive border không |

Mỗi lần vẽ dùng một **preset**: profile theo band nhiệt độ và draw config. Vẽ frame dùng draw preset của intent
(`TEXT`, `UI`, `PAGE`); draw preset có thêm debt weight và có đặt edge flag không. Cleanup dùng cleanup preset của
loại cleanup (`ink_display_cleanup_t`); cleanup không cộng debt nên cleanup preset không có hai field này. Preset nằm ở
display, không nằm trong profile, nên một waveform dùng được cho nhiều preset.

Intent, loại cleanup và rect nằm trong `ink_display_types.h`, header chỉ có kiểu; policy, display và HAL cùng dùng,
không chép qua lại. Decision có `action` (`NONE`, `DRAW`, `CLEANUP`), kèm `intent` khi `DRAW` hoặc `cleanup` khi
`CLEANUP`. Giá trị 0 của `action` và của `cleanup` là `INVALID`, nên decision quên điền bị policy và display từ chối.
Policy tạo decision bằng hàm riêng cho từng action, không điền struct tay.

| Preset | Dùng cho | Old plane | Window |
|---|---|---|---|
| Draw `TEXT`, `UI`, `PAGE` | Vẽ frame theo intent | Shown frame | Changed window |
| `CLEANUP_STRONG` | Cold boot, sau lỗi, vượt debt ceiling ngoài active period, `PAGE` khi debt vượt `debt_page`, stage cuối, trước khi ngủ | Frame mới đảo bit | Cả panel |
| `CLEANUP_FAST` | Vượt debt ceiling trong active period; frame có flag `REDRAW_WINDOW` | Frame mới, đảo bit ở pixel có debt hoặc edge flag trong changed window (gồm pixel frame này đổi) | Bao các pixel đó |
| `CLEANUP_EDGE_CENTER` | Cleanup stage 1 | Shown frame, đảo bit ở pixel có edge flag | Bao các pixel có edge flag |
| `CLEANUP_EDGE_NEIGHBOR` | Cleanup stage 1, ngay sau edge center | Shown frame, đảo bit ở pixel kề cạnh pixel có edge flag | Bao các pixel đó |
| `CLEANUP_LIGHT` | Cleanup stage 2 | Shown frame, đảo bit ở pixel có debt ≥ `debt_light` | Bao các pixel đó |

Cleanup fast chỉ kéo pixel bẩn (có debt hoặc edge flag) nằm trong changed window, và vẽ luôn frame mới. Policy cộng
debt cho pixel frame này đổi trước, nên chúng nằm trong số pixel bẩn. Pixel sạch (chữ đứng yên quanh ô vừa lật) giữ
LUT giữ màu 0 V, nên không bị lắc. Pixel bẩn ngoài window cũng không bị lắc: vẽ một trang chữ bằng `TEXT` đã cộng
debt 1 cho mọi pixel chữ, nên không giới hạn window thì mỗi lần cleanup fast lắc cả trang. Các pixel đó, và edge
flag ngay ngoài window, để các cleanup stage xử lý. Đo trên BB0 (2026-11-09): cleanup fast cả panel bằng `clean_full100` làm chữ đứng yên quanh ô lật nhiều
lần xám tới mức không đọc được.

Cleanup fast dùng profile `clean_full50` (640 ms): pixel được chọn lắc 15 LUT frame về màu ngược rồi 15 LUT frame về
màu của nó, 50 Hz, cân bằng điện tích ngay trong lần đó. Mỗi chiều bằng một lần kéo của `tr8`; `clean_full100` chỉ
bằng một nửa. Đo trên BB0: `tr8` (kéo một chiều) làm vùng không đổi đậm dần sau mỗi lần cleanup, nên không dùng làm
cleanup fast.

Drive border bật ở cleanup strong và `PAGE`. Đo trên BB0: không drive border thì border xám dần, kể cả trong partial
mode. `TEXT`, `UI`, cleanup fast và các cleanup stage để border nổi.

Cleanup strong và cleanup fast đọc lại nhiệt độ trước khi vẽ; bảng `READS_TEMPERATURE` trong `ink_display.c` giữ
điều này theo loại cleanup.

Old plane của mọi cleanup cả panel là frame mới đảo bit, không bao giờ là shown frame. Khi đó mọi pixel được coi là
đổi màu, nên panel kéo hết pixel về đúng màu. Dùng shown frame thì waveform chỉ kéo pixel đổi màu, và ảnh vẫn mờ sau
cleanup (đã gặp trên BB0 với band TR6).

## Decide

Phần quyết định nằm trong khối C thuần `display_policy`, có host test. Policy so frame mới với shown frame, lấy changed
window (mép x căn theo `x_align`), rồi xét các điều kiện theo thứ tự. Gặp điều kiện nào đúng trước thì dừng:

| # | Điều kiện | Decision |
|---|---|---|
| 1 | Chưa biết panel đang hiện gì (cold boot), hoặc lần vẽ trước lỗi | Cleanup strong |
| 2 | Flag `BEFORE_SLEEP`, và còn debt hoặc còn edge flag | Cleanup strong |
| 3 | Frame giống hệt shown frame | Không vẽ |
| 4 | Lần vẽ đầu sau wake, shown frame là background frame | Preset của intent, window cả panel |
| 5 | Flag `REDRAW_WINDOW` | Cleanup fast. Mọi pixel trong changed window (đã căn theo `x_align`) được cộng debt ít nhất 1, kể cả pixel không đổi màu, nên cleanup fast chọn cả vùng đó |
| 6 | Một pixel trong changed window sẽ vượt debt ceiling | Trong active period: cleanup fast (chỉ pixel bẩn); ngoài: cleanup strong. Cả hai vẽ luôn frame này, không cleanup trước rồi mới vẽ |
| 7 | Intent `PAGE` và debt lớn nhất ≥ `debt_page` | Cleanup strong |
| 8 | Còn lại | Preset của intent, chỉ changed window |

Sau wake, display coi background frame là shown frame: máy đã vẽ nó ngay trước deep sleep, và panel e-ink giữ hình khi
mất nguồn. Driver nhận background frame làm old plane ở lần vẽ đó, nên panel không chớp.

## pixel_state

Mỗi pixel có một byte trong PSRAM (240 × 416 pixel, khoảng 100 KB), do display cấp và `display_policy` giữ. Chỉ CPU
đọc byte này; hai plane gửi cho driver nằm trong RAM nội.

| Trường | Nghĩa |
|---|---|
| Edge flag (2 bit) | Không có, dark edge, light edge |
| Debt (4 bit) | Mỗi lần pixel đổi màu bằng preset của intent, debt cộng debt weight của preset; tối đa 15 |

**Edge flag.** Pixel không đổi nằm cạnh pixel vừa đổi màu bị điện trường lan sang, đổi màu một phần và để lại viền.
Luật đánh dấu theo bằng sáng chế US 11,568,827 của E Ink, chạy ở mỗi lần vẽ có preset bật `marks_edges`. Mặc định chỉ
`UI` bật. Lý do: đổi một trang chữ đặt hàng nghìn edge flag (đo trên BB0: 7 929), và cleanup edge center lắc từng pixel
có flag nên cả trang chớp lấm tấm. Viền của `TEXT`, `PAGE` để cleanup strong xoá.

| Flag | Điều kiện |
|---|---|
| Dark edge | Pixel đi trắng → trắng; cả 4 pixel kề cạnh (trên, dưới, trái, phải) trong frame mới đều trắng; ít nhất 1 pixel kề cạnh trong shown frame không trắng |
| Light edge | Pixel đi đen → đen; ít nhất 1 pixel kề cạnh đi trắng → đen |
| Chung | Không pixel nào trong 8 pixel xung quanh đã có flag |

Luật "Chung" làm flag thưa: dọc một cạnh thẳng, cứ một pixel có flag thì pixel bên cạnh không có. Pixel có flag là
tâm (center); các pixel kề cạnh tâm được xử lý ở cleanup edge neighbor.

**Debt và debt ceiling co giãn.** Debt không tự giảm theo thời gian; chỉ cleanup mới xoá. Debt ceiling bắt đầu ở
`debt_ceiling`. Frame tới trong vòng `active_ms` sau frame trước là active period: debt ceiling tăng 1, tối đa
`debt_ceiling_max`, để người dùng ít bị ngắt giữa chừng; vượt ceiling trong active period thì cleanup fast thay cho
cleanup strong. Idle quá `active_ms` thì debt ceiling về `debt_ceiling`.

Deep sleep làm mất PSRAM, và RTC memory (8 KB slow + 8 KB fast) không chứa nổi 100 KB. Vì vậy panel phải sạch trước khi ngủ (flag
`BEFORE_SLEEP`), và sau wake mọi pixel bắt đầu với debt 0, đúng với panel thật.

Đo trên BB0 (CPU 240 MHz, `-O2`, PSRAM 80 MHz): đổi cả một trang chữ thì so frame, cập nhật pixel_state và dựng old
plane mất 11 ms.

Lúc task khác ghi flash, cache tắt nên PSRAM không đọc được: display task dừng tới khi ghi xong. Lần vẽ trùng lúc đó
chậm thêm đúng thời gian ghi.

## Cleanup khi idle

Không có frame mới thì display task chạy các cleanup stage, tính từ lần vẽ frame gần nhất. Có frame mới thì các stage
bắt đầu lại. Idle càng lâu thì cleanup càng mạnh, vì người dùng càng có khả năng đã rời mắt, và điện áp dư trong mực
đã rã.

| Stage | Khi | Làm gì | Profile |
|---|---|---|---|
| 1 | Idle `edge_ms`, còn edge flag | Cleanup edge center, rồi cleanup edge neighbor; xoá edge flag | Center: DRF 320 ms; neighbor: DRF dưới 100 ms |
| 2 | Idle `light_ms`, debt lớn nhất ≥ `debt_light` | Cleanup light các pixel có debt ≥ `debt_light`; debt của chúng về 0 | DRF dưới 100 ms |
| 3 | Idle `strong_ms`, còn debt hoặc còn edge flag | Cleanup strong một lần; mọi debt và edge flag về 0 | Theo band nhiệt độ |

- Mỗi stage chạy tối đa một lần trong một idle period. Stage có thời gian 0 thì bỏ.
- DRF không ngắt được. Frame mới tới giữa hai cleanup thì dừng các stage và vẽ frame đó. Lần lật trùng lúc cleanup
  chậm thêm tối đa một DRF cleanup, nên profile của cleanup edge neighbor và cleanup light phải có DRF dưới 100 ms.
  Cleanup edge center là ngoại lệ: khảo sát trên BB0 cho thấy center dài (`clean_full100`, 320 ms) làm viền khó thấy
  hơn hẳn, đổi lại lần lật trùng cleanup này chậm thêm tối đa 320 ms. Cleanup này chỉ chạm pixel có flag (thưa), nên
  ít khi trùng.
- Cleanup edge center xong mà chưa kịp cleanup edge neighbor thì edge flag giữ nguyên; idle period sau chạy lại cả hai.
- Flag `DEFER_CLEANUP` của frame gần nhất chặn cả ba stage.

**Tắt analog power.** Driver không tự tắt analog power sau DRF. Sau `power_off_ms` không có DRF, display gọi
`power_off`; lần vẽ sau driver tự bật lại (khoảng 75 ms). Trong active period analog power giữ bật, nên mỗi lần lật
bớt được đoạn bật nguồn. Trước khi ngủ luôn tắt analog power.

## Nhiệt độ

Nhiệt độ đổi độ nhớt của mực. Profile không mang nhiệt độ; mỗi preset có một danh sách band: nhiệt độ tối đa (°C) →
id profile, xét từ trên xuống. Display đọc cảm biến của controller trước mỗi cleanup strong và cleanup fast, rồi dùng
nhiệt độ đó cho mọi preset tới lần đọc sau.

- Chưa đọc được lần nào thì dùng 25 °C.
- Số đọc ngoài −10…60 °C thì bỏ, giữ nhiệt độ cũ.
- Lệnh `0x40` khi REG = 1 đọc được trên BB0, lệch nhiệt độ phòng khoảng 2–3 °C (2026-11-04).
- Nhiệt độ được ghi vào report của lần vẽ có đọc.

Band mặc định của cleanup strong dùng 9 profile chép từ band TR0–TR8 của OTP, với đúng ngưỡng của OTP (5, 10, 15, 20,
30, 80, 92, 100, 127 °C). Các preset khác mặc định có một band.

## Chia việc giữa display và driver

| Việc | Ai làm |
|---|---|
| Giữ shown frame, so frame, drop frame pending ở giữa, pixel_state, chọn preset, dựng old plane, cleanup stage, tắt analog power khi idle, chọn profile theo nhiệt độ | Display; phần quyết định viết bằng C thuần để test trên PC |
| Ghi new plane và old plane vào đúng vùng RAM theo lượt, nạp profile được chỉ định, bật analog power, tốc độ SPI, đọc cảm biến nhiệt | Driver |

Driver không biết shown frame là gì. Nó chỉ nhận hai plane bit cho mỗi pixel: bit mới và bit cũ. Controller dùng cặp
bit (mới, cũ) của từng pixel để chọn một trong bốn LUT: trắng → đen, đen → trắng, trắng giữ trắng, đen giữ đen. Vì vậy
display truyền những cặp plane không phải "ảnh mới, ảnh cũ" để ra lệnh riêng cho từng pixel: mới = cũ thì pixel chạy
LUT giữ màu; mới = cũ đảo bit thì pixel bị kéo về màu của bit mới.

## Hợp đồng driver màn

Ví dụ: display muốn đổi một ô chữ ở góc trên. Nó gọi `draw` của driver với con trỏ tới frame mới, con trỏ tới shown
frame, window (x 0–79, y 0–39), `profile_index` và draw config. Driver nạp profile nếu khác lần trước, cắt 40 hàng ×
10 byte của window ở cả hai frame, ghi vào hai vùng RAM theo lượt, chạy DRF, chờ BUSY lên cao, rồi trả về timing.

`draw` chạy đồng bộ: gửi ảnh, ngủ chờ ngắt BUSY, xong mới trả về. Chỉ display task gọi, nên không chặn task khác.

| Điều khoản | Quy định |
|---|---|
| Toạ độ | Theo hướng gốc của panel (panel 3,7" là 240 × 416 px, dọc). Xoay màn do tầng trên lo |
| Frame | 1 bit mỗi pixel, bit 1 = trắng, bit 0 = đen. Gói theo hàng, pixel trái nhất ở bit cao nhất của byte |
| Ảnh đưa vào | Hai con trỏ tới cả frame (new plane, old plane) kèm window; driver tự cắt window, display không chép ảnh |
| Lượt ghi | Driver đếm số DRF từ lần reset. Số chẵn: new plane vào `0x13`, old plane vào `0x10`; số lẻ thì ngược lại. Ghi cả hai trước DRF; sau DRF không ghi thêm |
| Sau reset | RAM ảnh của controller là rác. Lần vẽ đầu sau reset, driver tự nới window ra cả panel |
| Profile | `profile_index` là index trong `capabilities.profiles` (không phải id). Driver luôn nạp đúng profile đó qua thanh ghi (REG = 1), không để controller chọn waveform theo nhiệt độ |
| Draw config | Đi kèm mỗi lần vẽ: `partial_mode` (PTL) hay vẽ cả panel; `scan_outside_window`; `drive_border`. Ngoài partial mode thì driver vẽ cả panel, bỏ qua window |
| Mép window | Driver làm tròn ra ngoài theo `x_align` khai báo trong capabilities |
| Analog power | Driver bật trước DRF nếu đang tắt, và để bật sau DRF. Tắt bằng `power_off`. Byte 2 của lệnh `0x00` là `0x0A` (TIEG 0, NORG 1, VC_LUTZ 0). Đo trên BB0: mặc định `0x0D` của UC8253 (TIEG 1, VGL nối đất sau POF) làm vùng trắng tối dần ngay lúc tắt nguồn. Lệnh `0x03` (PFS) giữ mặc định 1 LUT frame; 1–4 LUT frame không khác nhau |
| Timeout chờ BUSY | `drf_ms` của profile × 1,5 + 100 ms cho mỗi lần quét |
| Timing trả về | Bốn số: `spi_us` (đoạn 5), `power_us` (bật analog power nếu đang tắt), `panel_us` (quét tới khi BUSY lên cao, đoạn 6), `post_scan_us` |
| Giữa các lần vẽ | Controller giữ thanh ghi và RAM ảnh, không deep sleep |

Ngoài `draw`, interface của driver có:

| Thao tác | Việc |
|---|---|
| `power_off` | Gửi POF nếu analog power đang bật |
| `read_temperature` | Đọc cảm biến trong controller (lệnh `0x40`, sai số ±2 °C) |

Capabilities của driver:

| Mục | Nghĩa |
|---|---|
| `width`, `height` | Cỡ theo hướng gốc của panel, px |
| `x_align` | Mép trái và mép phải window là bội của số này |
| `profiles` | Id, tên, source (band TR nào của OTP, hoặc `SOURCE_CUSTOM`) và thời gian một lần vẽ của từng profile |
| `draw_defaults`, `cleanup_defaults` | Với mỗi intent (bỏ `AUTO`) và mỗi loại cleanup (bỏ `INVALID`): danh sách band → id profile, và draw config. Display dựng preset mặc định từ đây |

## Profile waveform

Ví dụ: cần partial nhanh hơn 343 ms. Dev thêm một profile vào `profiles.json` của driver, chạy `tools/profile_gen.py`
để kiểm và sinh lại file C, build và nạp. Sau đó gán id của profile đó cho preset `UI` trong options.

Driver không dùng waveform OTP lúc chạy: REG = 1 trong lệnh `0x00` ở mọi lần vẽ, không gửi `E0`/`E5`. Lý do:
controller tự chọn band theo nhiệt độ thì firmware không biết waveform nào đang chạy. Nạp lại band TR8 qua thanh ghi
cho DRF 343,1 ms, gần như OTP (344,6 ms).

Một profile gồm:

| Mục | Nghĩa |
|---|---|
| Id | Số cố định, không đổi khi đổi tên hay đổi thứ tự. Options lưu id, không lưu index |
| Tên, source | Tên để log và cho bench; source là band TR0–TR8 của OTP hoặc `SOURCE_CUSTOM` |
| 5 LUT | LUTC, LUTWW, LUTKW, LUTWK, LUTKK, mỗi LUT 42 byte |
| Điện áp | VG, VSH, VSL, VDHR, VCOM_DC, LUT frame rate (PLL), EOPT và XON của riêng profile |
| Số lần quét | 1–4 DRF cho một lần vẽ, mỗi DRF đẩy lại cùng cặp bit |
| `drf_ms` | Script tính từ LUT và PLL; driver dùng làm thời gian điển hình và để đặt timeout chờ BUSY |

Điện áp đi theo từng profile, vì OTP mỗi band có VCOM riêng: `0x13` (−2,0 V) ở TR0–TR7, `0x09` (−1,0 V) ở TR8
(VCOM = −0,1 × (mã + 1) V, UC8253 tr.37). Chép waveform của TR0–TR7 mà giữ VCOM của TR8 thì lệch 1 V. Profile custom lấy
điện áp của một band làm gốc.

Danh sách profile:

| Id | Profile | Waveform | Một lần vẽ | Dùng cho |
|---|---|---|---|---|
| 1–9 | `tr0`–`tr8` | Chép nguyên band TR0–TR8 của OTP | 340 ms (TR8) tới 6,8 s (TR0) | Cleanup strong theo band nhiệt độ; `tr8` cho `TEXT`, `PAGE` |
| 13 | `lut10f50` | Pixel đổi màu kéo 10 LUT frame, 50 Hz; giữ màu 0 V | 240 ms | `UI` |
| 21 | `clean_edge` | Pixel được chọn lắc 2–1–1–1–1–2 LUT frame, kết thúc ở màu của nó, 200 Hz; giữ màu 0 V | 50 ms | Cleanup edge neighbor |
| 22 | `clean_light` | Pixel được chọn: 4 LUT frame về màu ngược, rồi 4 LUT frame về màu của nó, 200 Hz; giữ màu 0 V | 50 ms | Cleanup light |
| 24 | `clean_full100` | Pixel được chọn: 15 LUT frame về màu ngược, rồi 15 LUT frame về màu của nó, 100 Hz; giữ màu 0 V | 320 ms | Cleanup edge center |
| 29 | `clean_full50` | Như `clean_full100`, 50 Hz (cùng waveform với id 23 đã bỏ) | 640 ms | Cleanup fast |

Id 10–12, 14–17, 20, 23, 25–28 là các profile đã thử bằng bench rồi bỏ; kết quả đo ở
`docs/hardware/parts/weact-epaper-3.7/refresh.md`. Thời gian một lần vẽ là số script tính, khớp số đo trên BB0 trong
khoảng 10 ms, chưa gồm khoảng 70 ms bật analog power khi nguồn đang tắt. Profile chép từ OTP và `lut10f50` kéo pixel
đổi màu theo một chiều; profile cleanup lắc qua lại để tổng điện tích bằng 0. Chọn theo khảo sát trên BB0: cleanup
edge center dài (`clean_full100`) cho viền khó thấy nhất; `tr8` đủ rõ cho chữ.

### Danh sách profile nằm ở đâu

| Nơi | Nội dung | Ai tạo |
|---|---|---|
| `components/drivers/ink_drv_uc8253/profiles/`: bản dump OTP của BB0 và `profiles.json` | Nguồn của danh sách: profile nào chép band nào, profile custom viết bằng các pha (mức điện áp, số LUT frame), mặc định theo intent và loại cleanup. `waveform_view.html` cùng thư mục vẽ các band của bản dump | Dev |
| `ink_drv_uc8253_profiles.c`: mảng C const trong flash | Sinh bằng `tools/profile_gen.py` từ nguồn trên; không sửa tay | Script, lúc dev chạy |

Vẽ waveform chỉ phục vụ phát triển: thử waveform mới, đổi waveform khi đổi panel, hoặc thêm kiểu hiển thị riêng cho
app. Người dùng chỉ chọn trong danh sách, nên firmware không đọc profile từ LittleFS.

Driver không đọc OTP lúc chạy. Panel khác thì dùng lệnh đọc OTP của bench (nhánh `bench`), lưu bản dump mới, rồi sinh
lại file. Chưa có tấm panel thứ hai để biết VCOM trong OTP có khác giữa các tấm không.

Driver nhớ profile đang nạp trong controller, và chỉ gửi lại LUT khi `profile_index` khác lần trước (khoảng 270 byte
SPI, dưới 1 ms ở 4 MHz). Reset controller thì quên profile đang nạp.

### Kiểm tra profile

Mọi profile đi qua `profile_gen.py` trước khi vào firmware, nên firmware không kiểm nội dung. Script từ chối profile
vi phạm:

| Luật | Lý do |
|---|---|
| Cân bằng điện tích: tổng (điện áp × số LUT frame) của LUTWK cộng LUTKW bằng 0 (VSH tính +1, VSL tính −1). LUT giữ màu lệch thì chỉ cảnh báo; profile chép từ OTP không kiểm; cờ `allow_unbalanced` cho bỏ qua | Một pixel đi trắng → đen → trắng chạy qua hai LUT; tổng khác 0 thì điện tích tích dần, hạt mực bị kẹt và để lại bóng lâu dài |
| Mọi profile custom, kể cả profile cleanup, phải cân bằng | Display không theo dõi điện tích lúc chạy |
| Số lần quét 1–4; số lặp nhóm và lặp trạng thái khác 0; mỗi pha tối đa 63 LUT frame; tối đa 6 nhóm | Nghĩa của lặp 0 chưa đo; giới hạn của định dạng LUT |
| LUT frame rate có trong bảng PLL của UC8253 | Lệnh `0x30` chỉ nhận các mức trong bảng |
| Điện áp nằm trong khoảng thanh ghi của UC8253; VG khoá ±20 V; không dùng mức VDHR trong 4 LUT nguồn | Spec panel không ghi giới hạn riêng |
| Id không trùng, không dùng lại id đã bỏ | Options cũ sẽ trỏ sai profile |

## Hiện tượng vật lý của mực và tầng xử lý

| Hiện tượng | Tầng xử lý |
|---|---|
| Điện trường của pixel đang đẩy lan sang pixel bên cạnh, để lại viền xám | `display_policy` đặt edge flag; cleanup stage 1 xử lý center và neighbor |
| Mất cân bằng điện tích làm hạt mực bị kẹt | `profile_gen.py` kiểm profile trước khi sinh file; mọi profile custom cân bằng |
| Hạt chưa tới đích, màu xám | Profile (số LUT frame, LUT frame rate); debt theo pixel, cleanup light và cleanup strong |
| Pixel không đổi bị LUT giữ màu kéo thêm | Profile custom có LUT giữ màu 0 V |
| Lúc tắt nguồn, điện áp dư làm vùng trắng tối đi | Driver đặt TIEG = 0 (byte 2 của lệnh `0x00`) |
| Dòng rò transistor TFT làm sụt điện áp trong một LUT frame | Phần cứng; phần mềm chỉ chỉnh LUT frame rate. Tần số càng cao thì mỗi LUT frame càng yếu |
| Nhiệt độ đổi độ nhớt của mực | Driver đọc cảm biến; display chọn profile theo band nhiệt độ của từng preset |
| Điện áp dư sau xung rã trong vài phút | Cleanup strong đặt ở stage cuối, khi đã idle lâu |

## Options và report

Ví dụ: người dùng chọn "lật nhanh" trong app Settings. App gọi `ink_display_set_options()` trên main task với preset
`UI` trỏ tới profile nhanh hơn. Display ghi options trong lock và báo cho display task; display task áp dụng trước frame
sau. Khi khởi động, `system` truyền options đã lưu vào `ink_display_start()`; chưa có thì dùng
`ink_display_default_options()`.

| Option | Nghĩa | Mặc định |
|---|---|---|
| `draw_presets[intent]`, `cleanup_presets[cleanup]` | Danh sách band → id profile; draw config. Draw preset có thêm `debt_weight` và `marks_edges`. Id không còn trong danh sách thì dùng mặc định của driver cho intent hoặc cleanup đó | Mặc định của driver; debt weight `TEXT` 1, `UI` 2, `PAGE` 1; `marks_edges` chỉ `UI` |
| `auto_ui_percent` | Ngưỡng diện tích để `AUTO` thành `UI` | 10 % |
| `debt_light`, `debt_page` | Ngưỡng của cleanup light, và của cleanup strong khi `PAGE` | 2, 6 |
| `debt_ceiling`, `debt_ceiling_max` | Debt ceiling sau idle, và debt ceiling lớn nhất trong active period | 8, 15 |
| `active_ms` | Frame tới trong khoảng này sau frame trước là active period: debt ceiling tăng 1, vượt ceiling thì cleanup fast; 0 = tắt | 2 s |
| `edge_ms`, `light_ms`, `strong_ms` | Thời gian idle của ba cleanup stage; 0 = bỏ stage đó | 2 s, 10 s, 60 s |
| `power_off_ms` | Không có DRF trong khoảng này thì tắt analog power; 0 = chỉ tắt trước khi ngủ | 4 s |

Các mặc định về debt và thời gian là điểm xuất phát, chưa đo; chỉnh bằng bench.

Sau mỗi lần display task làm việc với panel (vẽ frame, một cleanup, tắt analog power), nó gọi callback `on_report`
(nếu có) với một report gồm: action (thêm `POWER_OFF`), intent hoặc loại cleanup, id profile, window, nhiệt độ nếu vừa đọc, lỗi, debt lớn nhất, số pixel
có edge flag, debt ceiling, và 6 mốc thời gian theo bảng "Một lần vẽ đi qua đâu". `input_ms` do main task đưa vào lúc
submit; các mốc khác display tự ghi. Report là sự kiện "vẽ xong" mà luồng đi ngủ chờ, và là nguồn số đo của bench.
Firmware chính không in log hay giữ thống kê thời gian.

## Khi nào xem lại

- Đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng): bàn chuyển sang gửi danh sách lệnh vẽ để display task dựng
  ảnh.
- Có app cần vẽ chuyển động liên tục (cuộn mượt, animation): bàn chế độ ngân sách xung theo từng pixel. Profile là
  một xung ngắn, mỗi pixel đếm số xung còn phải kéo, display task quét lặp và đổi đích giữa chừng.
- App cần intent khác nhau cho từng vùng trong cùng một frame: bàn gắn intent theo vùng.
- Pixel vừa đổi về trắng đậm dần khi để yên (đo trên BB0 với `tr8`, nhẹ hơn sau khi đặt TIEG = 0) gây khó chịu trước
  khi cleanup stage kịp chạy: so `tr8` với LUT cân bằng trong từng lần đổi màu (như `clean_full100`) bằng bài "VCOM
  và vùng đứng yên" của bench.
- `UI` cần profile nhanh hơn mà xám nhiều, hoặc trộn `UI` (10 LUT frame) với `TEXT` (15 LUT frame) làm lệch điện tích thấy
  được: bàn đẩy bù. Sau khi vẽ nhanh, đẩy thêm một chiều về màu của pixel, kèm bộ đếm điện tích từng pixel. Thử tay
  trên BB0 (`lut10f50` rồi 5 LUT frame đẩy): sạch, không chớp, nhưng ô được đẩy đậm hơn xung quanh.
