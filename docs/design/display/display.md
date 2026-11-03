# Đường vẽ màn

## Vấn đề

Lật thẻ phải nhanh (≤ 0,3 s rồi giảm dần), và muốn tối ưu thì phải biết thời gian đi vào đâu. Gần hết thời gian là
lúc panel quét, nằm trong driver màn. Bảng sóng trong OTP của controller UC8253 do controller tự chọn theo nhiệt độ;
dải nhanh nhất cho partial 344 ms. Bảng sóng nạp qua thanh ghi xuống được 31–283 ms nhưng dễ để lại màu xám hoặc bóng
ảnh cũ. Chất lượng của từng bảng sóng phải nhìn bằng mắt, nên firmware phải biết chính xác bảng nào đang chạy và đổi
được bảng lúc máy đang chạy.

Controller có hai vùng RAM ảnh, ghi bằng lệnh `0x10` và `0x13`. Sau mỗi lần làm mới (DRF), hai vùng đổi vai "ảnh mới"
và "ảnh cũ" cho nhau. Driver chỉ ghi một vùng trước DRF thì mọi lần DRF thứ chẵn kể từ reset không đổi hình.

## Sơ đồ

Sơ đồ cùng thư mục, vẽ từ file này, mở bằng trình duyệt:


| File                | Cho thấy                                                                                                                           |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `architecture.html` | Các khối của đường vẽ màn và ai gọi ai                                                                                    |
| `sequence.html`     | Một lần lật thẻ: mượn và giao khung, chọn cách vẽ, driver ghi hai vùng RAM theo lượt, DRF, BUSY                        |
| `dataflow.html`     | Dữ liệu đổi dạng qua từng tầng: khung 1 bit → window và hai mặt phẳng → RAM → cặp bit chọn bảng sóng → điện áp |

Đổi đường vẽ màn thì vẽ lại ba sơ đồ trong cùng commit.

## Một lần vẽ đi qua đâu


| # | Đoạn                                                                          | Ai làm             | Task         |
| --- | --------------------------------------------------------------------------------- | --------------------- | -------------- |
| 1 | Bấm → cử chỉ → intent                                                      | `gesture`, `keymap` | main         |
| 2 | App vẽ vào canvas (ảnh 1 bit cả màn)                                       | app + `ui`          | main         |
| 3 | Giao khung cho task màn qua vùng đệm "khung mới nhất" (3 khung, RAM nội) | `display`           | main → màn |
| 4 | So với khung đang hiện, tìm vùng đổi, chọn partial/full                 | `display`           | màn         |
| 5 | Gửi ảnh mới và ảnh đang hiện của vùng đổi qua SPI                    | driver              | màn         |
| 6 | Panel quét, chờ chân BUSY                                                    | driver              | màn         |

Bấm nhanh hơn tốc độ màn thì các khung giữa bị bỏ, chỉ khung mới nhất được vẽ.

## Giao khung giữa task main và task màn

Ví dụ: app lật thẻ. Task main mượn một khung, khung này đã có sẵn ảnh của lần giao gần nhất, nên app chỉ cần vẽ phần
đổi. Vẽ xong, task main giao khung kèm thời điểm bấm nút. Task màn thức dậy, lấy khung đó ra vẽ.

Vùng đệm có 3 khung trong RAM nội. Lúc nào mỗi khung cũng có đúng một chủ:


| Chủ       | Khung                                                         |
| ------------ | --------------------------------------------------------------- |
| Task main  | Khung đang vẽ (đã mượn, chưa giao)                     |
| Hàng chờ | Khung mới nhất đã giao, task màn chưa lấy              |
| Task màn  | Khung đang hiện, và khung đang vẽ ra màn trong lúc vẽ |

- Task màn giữ khung đang hiện, vì driver cần nó làm ảnh cũ ở mỗi lần vẽ. Vẽ xong, khung vừa vẽ thành khung đang
  hiện và khung đang hiện cũ được trả về.
- Trong lúc task màn giữ hai khung, task main mượn thì lấy lại khung trong hàng chờ; khung đó tính là bị bỏ.
- Giao khung khi hàng chờ đã có khung thì khung cũ trong hàng chờ bị bỏ và được đếm.
- Mượn lần hai khi chưa giao khung đã mượn là lỗi của bên gọi.

Việc đổi chủ nằm trong khối C thuần `frame_slots`, có host test. Khối này chỉ giữ chủ của từng khung, không giữ bộ
nhớ ảnh. `display` gọi nó trong lock, rồi chép ảnh vào khung vừa mượn ở ngoài lock: khung giao gần nhất chỉ được trả
về khi đã có khung mới hơn, nên đọc nó ngoài lock vẫn an toàn.

## Chọn cách vẽ

Phần quyết định nằm trong khối C thuần `display_policy`, có host test. Nó so khung mới với khung đang hiện, lấy hình
chữ nhật bao vùng đổi (mép x căn theo bước căn của driver), rồi chọn theo thứ tự, gặp điều kiện nào trước thì dừng:


| # | Điều kiện                                                                                 | Cách vẽ                 |
| --- | ---------------------------------------------------------------------------------------------- | --------------------------- |
| 1 | Chưa biết màn đang hiện gì (khởi động nguội), hoặc lần vẽ trước bị lỗi      | Full                      |
| 2 | Lần vẽ đầu sau khi thức dậy, khung đang hiện là màn nền đã vẽ trước khi ngủ | Partial cả màn          |
| 3 | Khung giống hệt khung đang hiện                                                          | Không vẽ                |
| 4 | Số lần partial liên tiếp đã tới ngưỡng bóng mờ                                    | Full, để làm sạch     |
| 5 | Vùng đổi lớn hơn ngưỡng % diện tích màn                                            | Full                      |
| 6 | Còn lại                                                                                    | Partial, chỉ vùng đổi |

Ngưỡng nằm trong struct cấu hình. Mặc định: bóng mờ 10 lần partial; vùng lớn 100 %, tức là tắt luật số 5, vì một
lần full mất khoảng 2 s còn partial vùng lớn vẫn nhanh hơn.

Khi thức dậy, `display` coi màn nền là khung đang hiện, vì máy đã vẽ nó ngay trước khi ngủ sâu và màn e-ink giữ hình
khi mất nguồn. Driver nhận màn nền làm ảnh cũ ở lần vẽ đó, nên màn không chớp.

Khi khởi động nguội, `display` không biết màn đang hiện gì. Lần full đầu tiên dùng mặt phẳng cũ là khung mới đảo bit,
nên mọi điểm đều được coi là đổi màu và panel kéo hết các điểm về đúng màu.

## Chia việc giữa display và driver


| Việc                                                                                                                                  | Ai lo                                                                  |
| ---------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| Giữ ảnh đang hiện, so khung, bỏ khung giữa, chọn partial/full, đếm bóng mờ, chọn profile và cấu hình vẽ cho từng lần vẽ, chọn profile theo nhiệt độ | `display`; phần quyết định viết bằng C thuần để test trên PC |
| Ghi ảnh mới và ảnh cũ vào đúng vùng RAM theo lượt, nạp profile được chỉ định, nguồn analog, tốc độ SPI, đọc cảm biến nhiệt | Driver                                                                 |

Driver không hiểu "ảnh đang hiện". Nó chỉ nhận hai mặt phẳng bit cho mỗi điểm: bit mới và bit cũ. Controller dùng cặp
bit (mới, cũ) của từng điểm để chọn một trong bốn bảng sóng: trắng → đen, đen → trắng, trắng giữ trắng, đen giữ đen.
Vì vậy `display` có thể truyền một cặp mặt phẳng không phải "ảnh mới, ảnh cũ" để ra lệnh riêng cho từng điểm (ví dụ
đẩy lại viền), mà driver không phải đổi.

## Hợp đồng driver màn

Ví dụ: `display` muốn đổi một ô chữ ở góc trên. Nó gọi hàm vẽ của driver với con trỏ tới khung mới, con trỏ tới khung đang hiện, vùng cần đổi (x 0–79, y 0–39), index profile và cấu hình vẽ partial. Driver cắt 40 hàng × 10 byte của vùng đó ở cả hai khung, ghi vào hai vùng RAM theo lượt, chạy bảng sóng, chờ BUSY lên cao rồi trả về thời gian.

- **Đồng bộ:** hàm vẽ gửi ảnh rồi ngủ chờ ngắt BUSY, xong mới trả về. Chỉ task màn gọi nên không chặn ai khác.


| Điều khoản       | Quy định                                                                                                                                                                                           |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Toạ độ           | Theo hướng gốc của panel (với màn 3,7" là 240 × 416, dọc). Xoay màn do tầng trên lo.                                                                                                     |
| Khung ảnh          | 1 bit mỗi điểm, bit 1 = trắng, bit 0 = đen. Gói theo hàng, điểm trái nhất ở bit cao nhất của byte.                                                                                     |
| Ảnh đưa vào     | Hai con trỏ tới cả khung (mặt phẳng mới, mặt phẳng cũ) kèm vùng cần vẽ; driver tự cắt vùng, `display` không phải chép ảnh.                                                       |
| Lượt ghi          | Driver đếm số DRF từ lần reset. Số chẵn: mặt phẳng mới vào `0x13`, mặt phẳng cũ vào `0x10`; số lẻ thì ngược lại. Cả hai được ghi trước DRF; sau DRF không ghi gì thêm. |
| Sau reset           | RAM ảnh của controller là rác. Lần vẽ đầu sau reset, driver tự nới vùng ra cả màn.                                                                                                      |
| Profile           | Index trong danh sách profile của driver. Driver luôn nạp đúng profile đó qua thanh ghi, không để controller chọn bảng theo nhiệt độ. |
| Cấu hình vẽ       | Đi kèm mỗi lần vẽ, tách khỏi profile để một bảng sóng dùng được cho cả full lẫn partial: cả màn hay window, `PT_SCAN`, byte CDI (màu viền), giữ nguồn analog sau khi quét. Vẽ cả màn thì luôn tắt nguồn sau khi quét. |
| Mép vùng          | Driver làm tròn ra ngoài theo bước căn mép x mà nó khai báo trong bảng khả năng.                                                                                                        |
| Thời gian trả về | Ba số: SPI trước khi quét (đoạn 5), panel quét tới khi BUSY lên cao (đoạn 6), việc sau khi quét (tắt nguồn analog).                                                                   |
| Giữa các lần vẽ | Controller giữ thanh ghi và RAM ảnh, không ngủ sâu. Nguồn analog tắt hoặc giữ bật tuỳ cấu hình vẽ.                                                                                           |

Ngoài hàm vẽ, interface của driver màn có:


| Thao tác         | Việc                                                                          |
| ------------------- | -------------------------------------------------------------------------------- |
| Đọc nhiệt độ | Đọc cảm biến trong controller (lệnh `0x40`, sai số ±2 °C)              |

Bảng khả năng của driver màn:


| Mục                | Nghĩa                                                               |
| --------------------- | ---------------------------------------------------------------------- |
| Cỡ gốc            | Rộng và cao theo hướng gốc của panel                           |
| Bước căn mép x  | Mép trái và mép phải vùng partial phải là bội của số này |
| Danh sách profile  | Id, tên, nguồn (dải TR nào của OTP, hoặc vẽ tay) và thời gian DRF của từng profile |
| Profile mặc định | Index profile mặc định cho full và cho partial |

## Profile bảng sóng

Ví dụ: muốn partial nhanh hơn 344 ms. Dev vẽ bảng 6 khung ở 50 Hz bằng công cụ, script kiểm rồi sinh lại file C, build
và nạp. Từ đó `display` truyền index của profile `lut6f50` khi vẽ partial; DRF mất khoảng 163 ms.

Driver không dùng bảng sóng OTP nữa (REG = 1 trong lệnh `0x00` ở mọi lần vẽ, không gửi `E0`/`E5`). Lý do: controller
tự chọn dải theo nhiệt độ thì firmware không biết bảng nào đang chạy; nạp lại bảng TR8 qua thanh ghi cho DRF 343,1 ms,
gần như OTP (344,6 ms).

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
(VCOM = −0,1 × (mã + 1) V, UC8253 tr.37). Chép sóng của TR0–TR7 mà giữ VCOM của TR8 thì lệch 1 V.

Profile không mang nhiệt độ. Nhiệt độ vẫn đổi độ nhớt của mực, nên `display` chọn profile theo bảng dải nhiệt độ
(phần "Hiện tượng vật lý"); driver chỉ đọc cảm biến.

### Danh sách profile nằm ở đâu

| Nơi | Nội dung | Ai tạo |
|---|---|---|
| Mảng C const trong flash, do script Python sinh | 9 profile chép nguyên TR0–TR8 từ bản dump OTP của panel, các profile vẽ tay, và profile mặc định cho full, cho partial | Dev, lúc build |
| Một ô profile tạm trong RAM, chỉ có ở firmware test | Profile gửi từ PC để thử mà không phải nạp lại firmware; mất khi reset | Dev, qua bench |

Vẽ sóng chỉ phục vụ phát triển: thử bảng mới, đổi bảng khi đổi panel, hoặc thêm kiểu hiển thị riêng cho app. Người
dùng chỉ chọn trong danh sách, nên firmware không đọc profile từ LittleFS.

Driver không đọc OTP lúc chạy. Panel khác thì dùng lệnh đọc OTP của firmware test, rồi sinh lại file. Chưa có tấm
panel thứ hai để biết VCOM trong OTP có khác giữa các tấm không.

Driver nhớ profile đang nạp trong controller và chỉ gửi lại bảng khi index khác lần trước (khoảng 270 byte SPI, dưới
1 ms ở 4 MHz). Reset controller hoặc nạp profile tạm thì quên profile đang nạp.

### Kiểm tra profile

Mọi profile đi qua script trước khi vào firmware, kể cả profile tạm của bench, nên firmware không kiểm nội dung. Script
từ chối profile vi phạm:

| Luật | Lý do |
|---|---|
| Cân bằng điện tích: tổng (điện áp × số khung) của LUTWK và LUTKW bằng 0, tính theo điện áp của chính profile; bảng giữ màu lệch chỉ cảnh báo; profile chép từ OTP không kiểm; cờ `allow_unbalanced` cho bỏ qua | Một điểm đi trắng → đen → trắng chạy qua hai bảng; tổng khác 0 thì điện tích tích dần, hạt mực bị kẹt và để lại bóng lâu dài |
| Số lần quét 1–4, số lặp nhóm và lặp trạng thái khác 0 | Nghĩa của lặp 0 chưa đo |
| Điện áp trong dải thanh ghi của UC8253, VG khoá ±20 V, không dùng mức VDHR trong 4 bảng nguồn | Spec panel không ghi giới hạn riêng |
| Id không trùng, không dùng lại id đã bỏ | Cài đặt cũ trỏ sai profile |

Firmware test chỉ kiểm khung gói profile tạm (mã nhận dạng, phiên bản, độ dài, CRC). Gói sai thì từ chối và giữ
profile tạm cũ.

## Hiện tượng vật lý của mực và tầng xử lý


| Hiện tượng                                                                            | Tầng xử lý                                                                                |
| ------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------- |
| Điện trường của điểm đang đẩy lan sang điểm bên cạnh, để lại viền xám | `display_policy` đếm partial rồi full; profile có xung giữ; `display` đẩy lại viền  |
| Mất cân bằng điện tích làm hạt mực bị kẹt                                     | Script kiểm profile trước khi sinh file                                                 |
| Hạt chưa tới đích, màu xám                                                        | Profile (số khung, tần số khung); `display_policy` full refresh                           |
| Dòng rò transistor TFT làm sụt điện áp trong một khung                           | Phần cứng; phần mềm chỉ chỉnh tần số khung                                           |
| Nhiệt độ đổi độ nhớt của mực                                                   | Driver đọc cảm biến; `display` chọn profile theo bảng dải nhiệt độ của từng slot |
| Bóng kéo đuôi khi hình chuyển động                                               | `display` và app                                                                            |

**Bảng dải nhiệt độ.** `display` đọc nhiệt độ trước mỗi lần full và chọn profile cho từng slot theo bảng dải. Bảng
có một dải 0–40 °C (dải nhiệt độ hoạt động của panel), mỗi slot một profile lấy từ cài đặt. Nhiệt độ đọc được ghi vào
báo cáo của lần vẽ đó.

**Đẩy lại viền.** Policy nới vùng đổi thêm 1–2 điểm. Ở các điểm viền không đổi màu, `display` đảo bit cũ, nên controller
coi đó là một lần đổi màu và đẩy điểm về đúng màu của nó. Độ rộng nằm trong cài đặt (`edge_margin`), mặc định 0 là tắt.

## Cài đặt và báo cáo

Ví dụ: người dùng chọn "lật nhanh" trong app Cài đặt. App gọi `ink_display_set_options` trên task main; `display` ghi
cài đặt trong lock và báo cho task màn, task màn áp dụng trước khung sau. Khi khởi động, `system` truyền cài đặt đã lưu
vào `ink_display_start`; chưa có thì dùng `ink_display_default_options`.

| Cài đặt | Nghĩa | Mặc định |
|---|---|---|
| `full_profile`, `partial_profile` | Id profile của slot full và partial; id không còn trong danh sách thì dùng mặc định | Profile mặc định của driver |
| `partials_before_full` | Ngưỡng bóng mờ | 10 |
| `edge_margin` | Độ rộng đẩy lại viền; 0 = tắt | 0 |

Sau mỗi khung, task màn gọi callback `on_drawn` (nếu có) với một báo cáo: cách vẽ (không vẽ, partial, full), window,
profile, nhiệt độ nếu vừa đọc, lỗi, và 6 mốc thời gian ở bảng "Một lần vẽ đi qua đâu". Thời điểm bấm do task main đưa vào
lúc giao khung; các mốc khác `display` tự ghi. Báo cáo này là sự kiện "vẽ xong" mà luồng đi ngủ chờ, và là nguồn số đo
của firmware test. Firmware chính không in log hay giữ thống kê thời gian.

## Khi nào xem lại

Nếu đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng) thì bàn chuyển sang gửi danh sách lệnh vẽ để task màn dựng ảnh.
