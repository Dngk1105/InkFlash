# Đường vẽ màn

## Vấn đề

Lật thẻ phải nhanh (≤ 0,3 s rồi giảm dần), chữ phải rõ, và màn không được hỏng dần vì bóng mờ hay lệch điện tích. Gần
hết thời gian vẽ là lúc panel quét, nằm trong driver màn. Bảng sóng nạp qua thanh ghi của controller UC8253 cho DRF
31–343 ms, nhưng bảng càng ngắn càng dễ để lại màu xám, viền hoặc bóng ảnh cũ. Không bảng sóng nào vừa nhanh vừa sạch,
nên phải chọn bảng theo nội dung đang vẽ và dọn màn vào lúc người dùng không để ý.

App không biết màn vẽ thế nào; nó chỉ biết mình đang vẽ gì (một trang chữ, một con trỏ menu, một màn mới). `display` cần
đúng thông tin đó để chọn bảng sóng.

Controller có hai vùng RAM ảnh, ghi bằng lệnh `0x10` và `0x13`. Sau mỗi lần làm mới (DRF), hai vùng đổi vai "ảnh mới"
và "ảnh cũ" cho nhau. Driver chỉ ghi một vùng trước DRF thì mọi lần DRF thứ chẵn kể từ reset không đổi hình.

## Sơ đồ

Sơ đồ cùng thư mục, vẽ từ file này, mở bằng trình duyệt:

| File | Cho thấy |
|---|---|
| `architecture.html` | Các khối của đường vẽ màn và ai gọi ai |
| `sequence.html` | Một lần lật thẻ rồi thang dọn khi nghỉ: giao khung kèm ý định, chọn preset, driver ghi hai vùng RAM theo lượt, DRF, dọn viền, tắt nguồn |
| `dataflow.html` | Dữ liệu đổi dạng qua từng tầng: khung 1 bit và ý định → trạng thái từng điểm → window và hai mặt phẳng → profile → RAM → cặp bit chọn bảng sóng → điện áp |

Đổi đường vẽ màn thì vẽ lại ba sơ đồ trong cùng commit.

## Một lần vẽ đi qua đâu

| # | Đoạn | Ai làm | Task |
|---|---|---|---|
| 1 | Bấm → cử chỉ → intent | `gesture`, `keymap` | main |
| 2 | App vẽ vào khung (ảnh 1 bit cả màn) | app + `ui` | main |
| 3 | Giao khung kèm ý định vẽ cho task màn qua vùng đệm "khung mới nhất" (3 khung, RAM nội) | `display` | main → màn |
| 4 | So với khung đang hiện, tìm vùng đổi, cập nhật trạng thái từng điểm, chọn preset và mặt phẳng cũ | `display` | màn |
| 5 | Gửi profile (nếu khác lần trước), mặt phẳng mới và mặt phẳng cũ của window qua SPI | driver | màn |
| 6 | Panel quét, chờ chân BUSY | driver | màn |

Bấm nhanh hơn tốc độ màn thì các khung giữa bị bỏ, chỉ khung mới nhất được vẽ. Giữa hai lần bấm, task màn chạy thang
dọn khi nghỉ (phần "Dọn khi nghỉ").

## Giao khung giữa task main và task màn

Ví dụ: app lật thẻ. Task main mượn một khung, khung này đã có sẵn ảnh của lần giao gần nhất, nên app chỉ cần vẽ phần
đổi. Vẽ xong, task main giao khung kèm thời điểm bấm nút, ý định `TEXT` và cờ. Task màn thức dậy, lấy khung đó ra vẽ.

Vùng đệm có 3 khung trong RAM nội. Lúc nào mỗi khung cũng có đúng một chủ:

| Chủ | Khung |
|---|---|
| Task main | Khung đang vẽ (đã mượn, chưa giao) |
| Hàng chờ | Khung mới nhất đã giao, task màn chưa lấy |
| Task màn | Khung đang hiện, và khung đang vẽ ra màn trong lúc vẽ |

- Task màn giữ khung đang hiện, vì driver cần nó làm ảnh cũ ở mỗi lần vẽ. Vẽ xong, khung vừa vẽ thành khung đang
  hiện và khung đang hiện cũ được trả về.
- Trong lúc task màn giữ hai khung, task main mượn thì lấy lại khung trong hàng chờ; khung đó tính là bị bỏ.
- Giao khung khi hàng chờ đã có khung thì khung cũ trong hàng chờ bị bỏ và được đếm. Ý định và cờ đi theo khung mới.
- Mượn lần hai khi chưa giao khung đã mượn là lỗi của bên gọi.

Việc đổi chủ nằm trong khối C thuần `frame_slots`, có host test. Khối này chỉ giữ chủ của từng khung, không giữ bộ
nhớ ảnh. `display` gọi nó trong lock, rồi chép ảnh vào khung vừa mượn ở ngoài lock: khung giao gần nhất chỉ được trả
về khi đã có khung mới hơn, nên đọc nó ngoài lock vẫn an toàn.

## Ý định vẽ

Mỗi khung mang đúng một ý định. Một DRF chỉ chạy một bộ bảng sóng trên một window, nên khung có cả chữ lẫn nút thì app
chọn ý định của phần quan trọng hơn.

| Ý định | App dùng khi | `display` ưu tiên |
|---|---|---|
| `AUTO` | Không muốn chọn | Window nhỏ hơn `auto_ui_percent` % diện tích màn thì coi là `UI`, còn lại là `TEXT` |
| `TEXT` | Trang chữ người dùng sẽ đọc (mặt thẻ) | Độ rõ của chữ; 0,3–0,4 s chấp nhận được |
| `UI` | Con trỏ, mục chọn, nút, thành phần nhỏ người dùng không nhìn kỹ | Nhanh; chịu xám nhẹ, thang dọn khi nghỉ làm sạch dần |
| `PAGE` | Đổi cả màn (sang scene khác) | Như `TEXT`; nợ đã cao thì dọn luôn trong lần vẽ này |

Cờ đi kèm khung:

| Cờ | Nghĩa |
|---|---|
| Hoãn dọn | Không chạy thang dọn khi nghỉ cho tới khung sau không có cờ này. Trần nợ vẫn bắt buộc, tắt nguồn vẫn chạy |
| Trước khi ngủ | Khung là màn nền. Còn nợ thì vẽ bằng dọn mạnh; vẽ xong thì tắt nguồn analog |

## Một thao tác vẽ

`display` không phân biệt full và partial. Mỗi lần gọi driver gồm bốn thứ, và "full" chỉ là một preset dọn có window cả
màn, mặt phẳng cũ đảo bit:

| Thứ | Nghĩa |
|---|---|
| Window | Hình chữ nhật cần vẽ; mép x căn theo bước căn của driver |
| Mặt phẳng cũ | Bit cũ cho từng điểm, chọn bảng sóng của điểm đó (phần "Chia việc giữa display và driver") |
| Profile | Bảng sóng, điện áp, tần số khung |
| Cấu hình vẽ | Vào chế độ partial (PTL) hay không, `PT_SCAN`, có kéo viền màn không |

Mỗi mục đích vẽ có một **preset**: profile theo dải nhiệt độ, cấu hình vẽ, trọng số nợ. Preset nằm ở `display`, không
nằm trong profile, nên cùng một bảng sóng dùng được cho nhiều mục đích.

| Mục đích | Dùng cho | Mặt phẳng cũ | Window |
|---|---|---|---|
| `TEXT`, `UI`, `PAGE` | Vẽ khung theo ý định | Khung đang hiện | Vùng đổi |
| Dọn mạnh | Khởi động nguội, sau lỗi, vượt trần nợ, cuối thang dọn, trước khi ngủ | Khung mới đảo bit | Cả màn |
| Dọn nhanh | `PAGE` khi nợ đã quá ngưỡng | Khung mới đảo bit | Cả màn |
| Dọn tâm viền | Bậc 1 của thang dọn | Khung đang hiện, đảo bit ở điểm có cờ viền | Bao các điểm có cờ |
| Dọn quanh viền | Bậc 1 của thang dọn, ngay sau dọn tâm | Khung đang hiện, đảo bit ở điểm kề cạnh điểm có cờ | Bao các điểm đó |
| Dọn nhẹ | Bậc 2 của thang dọn | Khung đang hiện, đảo bit ở điểm có nợ xám | Bao các điểm đó |

Mặt phẳng cũ của mọi lần dọn cả màn là khung mới đảo bit, không bao giờ là khung đang hiện: mọi điểm được coi là đổi
màu, nên panel kéo hết các điểm về đúng màu. Dùng khung đang hiện thì bảng sóng chỉ kéo những điểm đổi màu, và ảnh vẫn
mờ sau lần dọn (đã gặp trên BB0 với bảng TR6).

## Chọn cách vẽ

Phần quyết định nằm trong khối C thuần `display_policy`, có host test. Nó so khung mới với khung đang hiện, lấy hình
chữ nhật bao vùng đổi (mép x căn theo bước căn của driver), rồi chọn theo thứ tự, gặp điều kiện nào trước thì dừng:

| # | Điều kiện | Cách vẽ |
|---|---|---|
| 1 | Chưa biết màn đang hiện gì (khởi động nguội), hoặc lần vẽ trước bị lỗi | Dọn mạnh |
| 2 | Cờ trước khi ngủ, và còn nợ hoặc còn cờ viền | Dọn mạnh |
| 3 | Khung giống hệt khung đang hiện | Không vẽ |
| 4 | Lần vẽ đầu sau khi thức dậy, khung đang hiện là màn nền đã vẽ trước khi ngủ | Preset của ý định, window cả màn |
| 5 | Một điểm trong vùng đổi sẽ vượt trần nợ | Dọn mạnh, vẽ luôn khung này (không dọn trước rồi mới vẽ) |
| 6 | Ý định `PAGE` và nợ lớn nhất đã tới `debt_page` | Dọn nhanh |
| 7 | Còn lại | Preset của ý định, chỉ vùng đổi |

Khi thức dậy, `display` coi màn nền là khung đang hiện, vì máy đã vẽ nó ngay trước khi ngủ sâu và màn e-ink giữ hình
khi mất nguồn. Driver nhận màn nền làm ảnh cũ ở lần vẽ đó, nên màn không chớp.

## Trạng thái của từng điểm

Mỗi điểm có một byte trong PSRAM (240 × 416 điểm, khoảng 100 KB), do `display` cấp và `display_policy` giữ. Chỉ CPU
đọc byte này; hai mặt phẳng gửi cho driver vẫn nằm trong RAM nội.

| Trường | Nghĩa |
|---|---|
| Cờ viền (2 bit) | Không có, viền tối, viền sáng |
| Nợ xám (4 bit) | Mỗi lần điểm đổi màu bằng preset của ý định, nợ của nó cộng trọng số của preset; tối đa 15 |

**Cờ viền.** Điểm đứng yên nằm cạnh điểm vừa đổi màu bị điện trường lan sang, đổi màu một phần và để lại viền. Luật
đánh dấu theo bằng sáng chế US 11,568,827 của E Ink, chạy trong mỗi lần vẽ theo ý định:

| Cờ | Điều kiện |
|---|---|
| Viền tối | Điểm đi trắng → trắng; cả 4 điểm kề cạnh (trên, dưới, trái, phải) trong khung mới đều trắng; ít nhất 1 điểm kề cạnh trong khung đang hiện không trắng |
| Viền sáng | Điểm đi đen → đen; ít nhất 1 điểm kề cạnh đi trắng → đen |
| Chung | Không điểm nào trong 8 điểm xung quanh đã có cờ |

Luật cuối làm cờ thưa (dọc một cạnh thẳng cứ một điểm có cờ thì điểm bên cạnh không có): điểm có cờ là tâm, các điểm
kề cạnh của tâm được dọn ở lượt dọn quanh viền.

**Nợ và trần co giãn.** Nợ không tự giảm theo thời gian, chỉ dọn mới hết. Trần nợ bắt đầu ở `debt_ceiling`. Khung tới
trong vòng `edge_ms` sau khung trước (người dùng đang thao tác liên tục) thì trần tăng 1, tối đa `debt_ceiling_max`, để
không bị dọn mạnh ngắt giữa chừng; nghỉ quá `edge_ms` thì trần về `debt_ceiling`.

Ngủ sâu làm mất PSRAM, và RTC memory (16 KB) không chứa nổi 100 KB. Vì vậy trước khi ngủ màn phải sạch (cờ trước khi
ngủ), và khi thức mọi điểm bắt đầu với nợ bằng 0, đúng với màn thật.

Lúc task khác ghi flash, cache tắt nên PSRAM không đọc được: task màn dừng tới khi ghi xong. Lần vẽ trùng lúc đó chậm
thêm đúng thời gian ghi.

## Dọn khi nghỉ

Không có khung mới thì task màn chạy một thang dọn, tính từ lần vẽ khung gần nhất. Có khung mới thì thang bắt đầu lại.
Nghỉ càng lâu thì dọn càng mạnh, vì người dùng càng có khả năng đã rời mắt, và điện áp dư trong mực đã rã.

| Bậc | Khi | Làm gì | Profile |
|---|---|---|---|
| 1 | Nghỉ `edge_ms`, còn cờ viền | Dọn tâm viền, rồi dọn quanh viền; xoá cờ | DRF dưới 100 ms |
| 2 | Nghỉ `light_ms`, nợ lớn nhất ≥ `debt_soft` | Dọn nhẹ các điểm có nợ ≥ `debt_soft`; nợ của chúng về 0 | DRF dưới 100 ms |
| 3 | Nghỉ `strong_ms`, còn nợ hoặc còn cờ | Dọn mạnh một lần; mọi nợ và cờ về 0 | Theo dải nhiệt độ |

- Mỗi bậc chạy tối đa một lần trong một lần nghỉ. Bậc có thời gian 0 thì bỏ.
- DRF không ngắt được. Khung mới tới giữa hai lượt dọn thì dừng thang và vẽ khung đó; lần lật trùng lúc đang dọn chậm
  thêm tối đa một DRF dọn, nên profile của bậc 1 và 2 phải có DRF dưới 100 ms.
- Dọn tâm xong mà chưa kịp dọn quanh thì cờ giữ nguyên, lần nghỉ sau dọn lại cả hai.
- Cờ hoãn dọn của khung gần nhất chặn cả ba bậc.

**Tắt nguồn analog.** Driver không tự tắt nguồn sau DRF. Sau `power_off_ms` không có DRF nào, `display` gọi tắt nguồn;
lần vẽ sau driver tự bật lại (khoảng 75 ms). Đang thao tác liên tục thì nguồn giữ bật, mỗi lần lật bớt được đoạn bật
nguồn. Trước khi ngủ luôn tắt nguồn.

## Nhiệt độ

Nhiệt độ đổi độ nhớt của mực. Profile không mang nhiệt độ; mỗi preset có một bảng dải: nhiệt độ tối đa (°C) → id
profile, xét từ trên xuống. `display` đọc cảm biến của controller trước mỗi lần dọn mạnh và dọn nhanh, rồi dùng nhiệt độ
đó cho mọi preset tới lần đọc sau. Chưa đọc được lần nào thì dùng 25 °C; số đọc ngoài −10…60 °C thì bỏ, giữ nhiệt
độ cũ (lệnh `0x40` khi REG = 1 chưa đo trên BB0). Nhiệt độ ghi vào báo cáo của lần vẽ có đọc.

Bảng dải mặc định của dọn mạnh dùng 9 profile chép từ các dải TR0–TR8 của OTP, với đúng ngưỡng của OTP (5, 10, 15, 20,
30, 80, 92, 100, 127 °C). Preset khác mặc định có một dải.

## Chia việc giữa display và driver

| Việc | Ai lo |
|---|---|
| Giữ ảnh đang hiện, so khung, bỏ khung giữa, trạng thái từng điểm, chọn preset, dựng mặt phẳng cũ, thang dọn, tắt nguồn khi nghỉ, chọn profile theo nhiệt độ | `display`; phần quyết định viết bằng C thuần để test trên PC |
| Ghi ảnh mới và ảnh cũ vào đúng vùng RAM theo lượt, nạp profile được chỉ định, bật nguồn analog, tốc độ SPI, đọc cảm biến nhiệt | Driver |

Driver không hiểu "ảnh đang hiện". Nó chỉ nhận hai mặt phẳng bit cho mỗi điểm: bit mới và bit cũ. Controller dùng cặp
bit (mới, cũ) của từng điểm để chọn một trong bốn bảng sóng: trắng → đen, đen → trắng, trắng giữ trắng, đen giữ đen.
Vì vậy `display` truyền những cặp mặt phẳng không phải "ảnh mới, ảnh cũ" để ra lệnh riêng cho từng điểm: mới = cũ thì
điểm chạy bảng giữ màu, mới = cũ đảo bit thì điểm bị kéo về màu của bit mới.

## Hợp đồng driver màn

Ví dụ: `display` muốn đổi một ô chữ ở góc trên. Nó gọi hàm vẽ của driver với con trỏ tới khung mới, con trỏ tới khung
đang hiện, vùng cần đổi (x 0–79, y 0–39), index profile và cấu hình vẽ. Driver nạp profile nếu khác lần trước, cắt
40 hàng × 10 byte của vùng đó ở cả hai khung, ghi vào hai vùng RAM theo lượt, chạy bảng sóng, chờ BUSY lên cao rồi trả
về thời gian.

- **Đồng bộ:** hàm vẽ gửi ảnh rồi ngủ chờ ngắt BUSY, xong mới trả về. Chỉ task màn gọi nên không chặn ai khác.

| Điều khoản | Quy định |
|---|---|
| Toạ độ | Theo hướng gốc của panel (với màn 3,7" là 240 × 416, dọc). Xoay màn do tầng trên lo. |
| Khung ảnh | 1 bit mỗi điểm, bit 1 = trắng, bit 0 = đen. Gói theo hàng, điểm trái nhất ở bit cao nhất của byte. |
| Ảnh đưa vào | Hai con trỏ tới cả khung (mặt phẳng mới, mặt phẳng cũ) kèm vùng cần vẽ; driver tự cắt vùng, `display` không phải chép ảnh. |
| Lượt ghi | Driver đếm số DRF từ lần reset. Số chẵn: mặt phẳng mới vào `0x13`, mặt phẳng cũ vào `0x10`; số lẻ thì ngược lại. Cả hai được ghi trước DRF; sau DRF không ghi gì thêm. |
| Sau reset | RAM ảnh của controller là rác. Lần vẽ đầu sau reset, driver tự nới vùng ra cả màn. |
| Profile | Index trong danh sách profile của driver. Driver luôn nạp đúng profile đó qua thanh ghi (REG = 1), không để controller chọn bảng theo nhiệt độ. |
| Cấu hình vẽ | Đi kèm mỗi lần vẽ: vào chế độ partial (PTL) hay vẽ cả màn ngoài chế độ partial; `PT_SCAN`; kéo viền màn hay để viền nổi. Ngoài chế độ partial thì driver vẽ cả màn, bỏ qua window. |
| Mép vùng | Driver làm tròn ra ngoài theo bước căn mép x mà nó khai báo trong bảng khả năng. |
| Nguồn analog | Driver bật nguồn trước DRF nếu đang tắt và để nguồn bật sau DRF. Tắt bằng thao tác tắt nguồn. |
| Thời gian chờ BUSY | Thời gian DRF của profile × 1,5 + 100 ms cho mỗi lần quét. |
| Thời gian trả về | Ba số: SPI trước khi quét (đoạn 5), panel quét tới khi BUSY lên cao (đoạn 6), việc sau khi quét. |
| Giữa các lần vẽ | Controller giữ thanh ghi và RAM ảnh, không ngủ sâu. |

Ngoài hàm vẽ, interface của driver màn có:

| Thao tác | Việc |
|---|---|
| Tắt nguồn | Gửi POF nếu nguồn đang bật |
| Đọc nhiệt độ | Đọc cảm biến trong controller (lệnh `0x40`, sai số ±2 °C) |

Bảng khả năng của driver màn:

| Mục | Nghĩa |
|---|---|
| Cỡ gốc | Rộng và cao theo hướng gốc của panel |
| Bước căn mép x | Mép trái và mép phải window phải là bội của số này |
| Danh sách profile | Id, tên, nguồn (dải TR nào của OTP, hoặc vẽ tay) và thời gian một lần vẽ của từng profile |
| Mặc định theo mục đích | Với mỗi mục đích vẽ: bảng dải nhiệt độ → id profile, và cấu hình vẽ. `display` dựng preset mặc định từ đây |

## Profile bảng sóng

Ví dụ: muốn partial nhanh hơn 343 ms. Dev thêm một profile vào `profiles.json` của driver, chạy `tools/profile_gen.py`
để kiểm và sinh lại file C, build và nạp. Từ đó gán id của profile đó cho preset `UI` trong cài đặt.

Driver không dùng bảng sóng OTP lúc chạy (REG = 1 trong lệnh `0x00` ở mọi lần vẽ, không gửi `E0`/`E5`). Lý do:
controller tự chọn dải theo nhiệt độ thì firmware không biết bảng nào đang chạy; nạp lại bảng TR8 qua thanh ghi cho DRF
343,1 ms, gần như OTP (344,6 ms).

Một profile gồm:

| Mục | Nghĩa |
|---|---|
| Id | Số cố định, không đổi khi đổi tên hay đổi thứ tự. Cài đặt lưu id, không lưu index |
| Tên, nguồn | Tên để log và bench; nguồn là dải TR0–TR8 của OTP hoặc vẽ tay |
| 5 bảng LUT | LUTC, LUTWW, LUTKW, LUTWK, LUTKK, mỗi bảng 42 byte |
| Điện áp | VG, VSH, VSL, VDHR, VCOM_DC, tần số khung (PLL), EOPT và XON của riêng profile |
| Số lần quét | 1–4 lần DRF cho một lần vẽ, mỗi lần đẩy lại cùng cặp bit |
| Thời gian DRF | Script tính từ bảng và PLL; driver dùng làm thời gian điển hình và để đặt timeout chờ BUSY |

Điện áp đi theo từng profile vì OTP mỗi dải có VCOM riêng: `0x13` (−2,0 V) ở TR0–TR7, `0x09` (−1,0 V) ở TR8
(VCOM = −0,1 × (mã + 1) V, UC8253 tr.37). Chép sóng của TR0–TR7 mà giữ VCOM của TR8 thì lệch 1 V. Profile vẽ tay lấy
điện áp của một dải làm gốc.

Danh sách profile:

| Id | Profile | Bảng sóng | Một lần vẽ | Dùng cho |
|---|---|---|---|---|
| 1–9 | `tr0`–`tr8` | Chép nguyên dải TR0–TR8 của OTP | 340 ms (TR8) tới 6,8 s (TR0) | Dọn mạnh theo dải nhiệt độ; `tr8` cho `TEXT`, `PAGE`, dọn nhanh |
| 10 | `lut15f65` | TR8, 65 Hz | 262 ms | |
| 11 | `lut15f100` | TR8, 100 Hz | 170 ms | |
| 12 | `lut12f50` | Kéo 12 khung, 50 Hz | 280 ms | |
| 13 | `lut10f50` | Kéo 10 khung, 50 Hz | 240 ms | `UI` |
| 14 | `lut6f50` | Kéo 6 khung, 50 Hz | 160 ms | |
| 15 | `lut4f200` | Kéo 4 khung, 200 Hz | 30 ms | |
| 16 | `3x5f50` | 3 lần quét, mỗi lần kéo 5 khung, 50 Hz | 420 ms | |
| 17 | `tr8_kk0` | TR8, bảng đen giữ đen 0 V | 340 ms | So với `tr8`: điểm đứng yên không bị kéo |
| 20 | `clean_full` | Điểm được chọn: 8 khung về màu ngược rồi 8 khung về màu của nó, 200 Hz; giữ màu 0 V | 90 ms | Dọn tâm viền |
| 21 | `clean_edge` | Điểm được chọn: lắc 2–1–1–1–1–2 khung, kết thúc về màu của nó, 200 Hz; giữ màu 0 V | 50 ms | Dọn quanh viền |
| 22 | `clean_light` | Điểm được chọn: 4 khung về màu ngược rồi 4 khung về màu của nó, 200 Hz; giữ màu 0 V | 50 ms | Dọn nhẹ |

Thời gian một lần vẽ là số script tính, chưa đo trên BB0. Bảng chép từ OTP và các bảng `lut*` có điểm chuyển màu kéo
một chiều; bảng dọn lắc qua lại để tổng điện tích bằng 0.

### Danh sách profile nằm ở đâu

| Nơi | Nội dung | Ai tạo |
|---|---|---|
| `components/drivers/ink_drv_uc8253/profiles/`: bản dump OTP của BB0 và `profiles.json` | Nguồn của danh sách: profile nào chép dải nào, profile vẽ tay viết bằng các pha (mức điện áp, số khung), mặc định theo mục đích. `waveform_view.html` cùng thư mục vẽ các dải của bản dump | Dev |
| `ink_drv_uc8253_profiles.c`, mảng C const trong flash | Sinh bằng `tools/profile_gen.py` từ nguồn trên; không sửa tay | Script, lúc dev chạy |
| Một ô profile tạm trong RAM của driver | Profile gửi từ PC để thử mà không phải nạp lại firmware (gói `.inkw` do script sinh); mất khi reset. Index bằng số profile cố định | Dev, qua bench |

Vẽ sóng chỉ phục vụ phát triển: thử bảng mới, đổi bảng khi đổi panel, hoặc thêm kiểu hiển thị riêng cho app. Người
dùng chỉ chọn trong danh sách, nên firmware không đọc profile từ LittleFS.

Driver không đọc OTP lúc chạy. Panel khác thì dùng lệnh đọc OTP của firmware test, lưu bản dump mới rồi sinh lại file.
Chưa có tấm panel thứ hai để biết VCOM trong OTP có khác giữa các tấm không.

Driver nhớ profile đang nạp trong controller và chỉ gửi lại bảng khi index khác lần trước (khoảng 270 byte SPI, dưới
1 ms ở 4 MHz). Reset controller hoặc nạp profile tạm thì quên profile đang nạp.

### Kiểm tra profile

Mọi profile đi qua script trước khi vào firmware, kể cả profile tạm của bench, nên firmware không kiểm nội dung. Script
từ chối profile vi phạm:

| Luật | Lý do |
|---|---|
| Cân bằng điện tích: tổng (điện áp × số khung) của LUTWK cộng LUTKW bằng 0 (VSH tính +1, VSL tính −1); bảng giữ màu lệch chỉ cảnh báo; profile chép từ OTP không kiểm; cờ `allow_unbalanced` cho bỏ qua | Một điểm đi trắng → đen → trắng chạy qua hai bảng; tổng khác 0 thì điện tích tích dần, hạt mực bị kẹt và để lại bóng lâu dài |
| Mọi profile vẽ tay, kể cả profile dọn, phải cân bằng | `display` không theo dõi điện tích lúc chạy |
| Số lần quét 1–4, số lặp nhóm và lặp trạng thái khác 0, mỗi pha tối đa 63 khung, tối đa 6 nhóm | Nghĩa của lặp 0 chưa đo; giới hạn của định dạng LUT |
| Tần số khung có trong bảng PLL của UC8253 | Lệnh `0x30` chỉ nhận các mức trong bảng |
| Điện áp trong dải thanh ghi của UC8253, VG khoá ±20 V, không dùng mức VDHR trong 4 bảng nguồn | Spec panel không ghi giới hạn riêng |
| Id không trùng, không dùng lại id đã bỏ | Cài đặt cũ trỏ sai profile |

Firmware test chỉ kiểm khung gói profile tạm (mã nhận dạng, phiên bản, độ dài, CRC). Gói sai thì từ chối và giữ
profile tạm cũ.

## Hiện tượng vật lý của mực và tầng xử lý

| Hiện tượng | Tầng xử lý |
|---|---|
| Điện trường của điểm đang đẩy lan sang điểm bên cạnh, để lại viền xám | `display_policy` đánh dấu cờ viền; thang dọn dọn tâm và quanh viền |
| Mất cân bằng điện tích làm hạt mực bị kẹt | Script kiểm profile trước khi sinh file; mọi profile vẽ tay cân bằng |
| Hạt chưa tới đích, màu xám | Profile (số khung, tần số khung); nợ xám theo điểm, dọn nhẹ và dọn mạnh |
| Điểm đứng yên bị bảng giữ màu kéo thêm | Profile có bảng giữ màu 0 V (`tr8_kk0`, profile dọn) |
| Dòng rò transistor TFT làm sụt điện áp trong một khung | Phần cứng; phần mềm chỉ chỉnh tần số khung. Tần số càng cao thì mỗi khung càng yếu |
| Nhiệt độ đổi độ nhớt của mực | Driver đọc cảm biến; `display` chọn profile theo bảng dải nhiệt độ của từng preset |
| Điện áp dư sau xung rã trong vài phút | Thang dọn để dọn mạnh ở bậc cuối, khi đã nghỉ lâu |

## Cài đặt và báo cáo

Ví dụ: người dùng chọn "lật nhanh" trong app Cài đặt. App gọi `ink_display_set_options` trên task main với preset `UI`
trỏ tới profile nhanh hơn; `display` ghi cài đặt trong lock và báo cho task màn, task màn áp dụng trước khung sau. Khi
khởi động, `system` truyền cài đặt đã lưu vào `ink_display_start`; chưa có thì dùng `ink_display_default_options`.

| Cài đặt | Nghĩa | Mặc định |
|---|---|---|
| Preset của từng mục đích | Bảng dải nhiệt độ → id profile; cấu hình vẽ; trọng số nợ (chỉ preset của ý định). Id không còn trong danh sách thì dùng mặc định của driver cho mục đích đó | Mặc định theo mục đích của driver; trọng số `TEXT` 1, `UI` 2, `PAGE` 1 |
| `auto_ui_percent` | Ngưỡng diện tích để `AUTO` thành `UI` | 10 % |
| `debt_soft`, `debt_page` | Ngưỡng của dọn nhẹ và của dọn nhanh khi `PAGE` | 2, 6 |
| `debt_ceiling`, `debt_ceiling_max` | Trần nợ lúc nghỉ và trần lớn nhất khi đang thao tác | 8, 15 |
| `edge_ms`, `light_ms`, `strong_ms` | Thời gian nghỉ của ba bậc thang dọn; 0 = bỏ bậc đó | 2 s, 10 s, 60 s |
| `power_off_ms` | Không có DRF chừng này thì tắt nguồn analog; 0 = chỉ tắt trước khi ngủ | 2 s |

Các mặc định về nợ và thời gian là điểm xuất phát, chưa đo; chỉnh bằng bench.

Sau mỗi lần task màn làm việc với màn (vẽ khung, một lượt dọn, tắt nguồn), nó gọi callback `on_drawn` (nếu có) với một
báo cáo: việc đã làm, mục đích, id profile, window, nhiệt độ nếu vừa đọc, lỗi, nợ lớn nhất, số điểm có cờ viền, trần
nợ, và 6 mốc thời gian ở bảng "Một lần vẽ đi qua đâu". Thời điểm bấm do task main đưa vào lúc giao khung; các mốc khác
`display` tự ghi. Báo cáo này là sự kiện "vẽ xong" mà luồng đi ngủ chờ, và là nguồn số đo của firmware test. Firmware
chính không in log hay giữ thống kê thời gian.

## Khi nào xem lại

- Nếu đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng) thì bàn chuyển sang gửi danh sách lệnh vẽ để task màn dựng ảnh.
- Nếu có app cần vẽ chuyển động liên tục (cuộn mượt, animation) thì bàn chế độ ngân sách xung theo từng điểm: profile
  là một xung ngắn, mỗi điểm đếm số xung còn phải kéo, task màn quét lặp và đổi đích giữa chừng.
- Nếu app cần ý định khác nhau cho từng vùng trong cùng một khung thì bàn gắn ý định theo vùng.
