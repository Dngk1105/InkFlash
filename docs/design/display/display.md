# Đường vẽ màn

## Vấn đề

Lật thẻ phải nhanh (≤ 0,3 s rồi giảm dần), và muốn tối ưu thì phải biết thời gian đi vào đâu. Gần hết thời gian là
lúc panel quét, nằm trong driver màn. Với bảng sóng trong OTP của controller UC8253, partial nhanh nhất mất 344 ms;
bảng sóng nạp qua thanh ghi xuống được 31–283 ms nhưng dễ để lại màu xám hoặc bóng ảnh cũ. Chất lượng của từng bảng
sóng phải nhìn bằng mắt, nên driver phải đổi được bảng sóng lúc máy đang chạy.

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
| Giữ ảnh đang hiện, so khung, bỏ khung giữa, chọn partial/full, đếm bóng mờ, chọn bảng sóng theo nhiệt độ              | `display`; phần quyết định viết bằng C thuần để test trên PC |
| Ghi ảnh mới và ảnh cũ vào đúng vùng RAM theo lượt, chạy bảng sóng, nguồn analog, tốc độ SPI, đọc cảm biến nhiệt | Driver                                                                 |

Driver không hiểu "ảnh đang hiện". Nó chỉ nhận hai mặt phẳng bit cho mỗi điểm: bit mới và bit cũ. Controller dùng cặp
bit (mới, cũ) của từng điểm để chọn một trong bốn bảng sóng: trắng → đen, đen → trắng, trắng giữ trắng, đen giữ đen.
Vì vậy `display` có thể truyền một cặp mặt phẳng không phải "ảnh mới, ảnh cũ" để ra lệnh riêng cho từng điểm (ví dụ
đẩy lại viền), mà driver không phải đổi.

## Hợp đồng driver màn

Ví dụ: `display` muốn đổi một ô chữ ở góc trên. Nó gọi hàm vẽ của driver với con trỏ tới khung mới, con trỏ tới khung đang hiện, vùng cần đổi (x 0–79, y 0–39) và slot bảng sóng `partial`. Driver cắt 40 hàng × 10 byte của vùng đó ở cả hai khung, ghi vào hai vùng RAM theo lượt, chạy bảng sóng, chờ BUSY lên cao rồi trả về thời gian.

- **Đồng bộ:** hàm vẽ gửi ảnh rồi ngủ chờ ngắt BUSY, xong mới trả về. Chỉ task màn gọi nên không chặn ai khác.


| Điều khoản       | Quy định                                                                                                                                                                                           |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Toạ độ           | Theo hướng gốc của panel (với màn 3,7" là 240 × 416, dọc). Xoay màn do tầng trên lo.                                                                                                     |
| Khung ảnh          | 1 bit mỗi điểm, bit 1 = trắng, bit 0 = đen. Gói theo hàng, điểm trái nhất ở bit cao nhất của byte.                                                                                     |
| Ảnh đưa vào     | Hai con trỏ tới cả khung (mặt phẳng mới, mặt phẳng cũ) kèm vùng cần vẽ; driver tự cắt vùng, `display` không phải chép ảnh.                                                       |
| Lượt ghi          | Driver đếm số DRF từ lần reset. Số chẵn: mặt phẳng mới vào `0x13`, mặt phẳng cũ vào `0x10`; số lẻ thì ngược lại. Cả hai được ghi trước DRF; sau DRF không ghi gì thêm. |
| Sau reset           | RAM ảnh của controller là rác. Lần vẽ đầu sau reset, driver tự nới vùng ra cả màn.                                                                                                      |
| Slot bảng sóng    | `full` hoặc `partial`. Mỗi slot trỏ tới một profile (phần "Profile bảng sóng").                                                                                                              |
| Mép vùng          | Driver làm tròn ra ngoài theo bước căn mép x mà nó khai báo trong bảng khả năng.                                                                                                        |
| Thời gian trả về | Ba số: SPI trước khi quét (đoạn 5), panel quét tới khi BUSY lên cao (đoạn 6), việc sau khi quét (tắt nguồn analog).                                                                   |
| Giữa các lần vẽ | Controller giữ thanh ghi và RAM ảnh, không ngủ sâu. Nguồn analog tắt hoặc giữ bật tuỳ profile.                                                                                           |

Ngoài hàm vẽ, interface của driver màn có:


| Thao tác         | Việc                                                                          |
| ------------------- | -------------------------------------------------------------------------------- |
| Chọn profile     | Gán một profile trong danh sách của driver cho slot `full` hoặc `partial` |
| Đọc nhiệt độ | Đọc cảm biến trong controller (lệnh `0x40`, sai số ±2 °C)              |

Bảng khả năng của driver màn:


| Mục                | Nghĩa                                                               |
| --------------------- | ---------------------------------------------------------------------- |
| Cỡ gốc            | Rộng và cao theo hướng gốc của panel                           |
| Bước căn mép x  | Mép trái và mép phải vùng partial phải là bội của số này |
| Danh sách profile  | Tên và thời gian quét điển hình của từng profile            |
| Profile mặc định | Profile gán cho slot `full` và slot `partial` lúc khởi tạo      |

## Profile bảng sóng

Ví dụ: muốn partial nhanh hơn 344 ms. Gõ `display profile set partial lut6f50`. Từ lần vẽ sau, driver nạp bảng sóng
6 khung ở 50 Hz qua thanh ghi, và DRF mất khoảng 163 ms. Màn hơi xám hơn, nhưng không còn bóng ô cờ cũ.

Một profile gồm:


| Mục                                     | Nghĩa                                                                                                                         |
| ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------- |
| Nguồn sóng                             | OTP ép nhiệt độ (`E0 02` + `E5`), OTP theo cảm biến (`E0 00`), hoặc LUT nạp qua thanh ghi (REG = 1 trong lệnh `0x00`) |
| Mã nhiệt độ                          | Giá trị gửi kèm `E5` khi ép nhiệt độ; 110 trở lên chọn dải nóng nhất TR8                                         |
| Số khung, tần số khung, số lần lặp | Chỉ dùng với LUT thanh ghi. Thời gian DRF = (số khung + 2) / tần số khung                                               |
| Xung giữ                                | Số khung đẩy lại cùng màu cho điểm không đổi màu (bảng đen giữ đen, trắng giữ trắng)                        |
| `PT_SCAN`, VBD                           | Cách quét partial, màu viền màn                                                                                           |
| Giữ nguồn analog                       | Bỏ POF sau khi quét và PON ở lần sau, bớt khoảng 75 ms mỗi lần                                                        |

Danh sách profile ban đầu lấy từ các bảng sóng đã đo trên BB0 (số đo của ô 80 × 40):


| Profile        | Bảng sóng                 | DRF            | Bóng mờ sau 100 lần đảo ô cờ |
| ---------------- | ----------------------------- | ---------------- | ------------------------------------- |
| `otp_std_full` | OTP theo cảm biến         | Khoảng 2,6 s  | Dùng cho full                      |
| `otp90_full`   | OTP ép 90                  | 2 055 ms       | Dùng cho full                      |
| `otp110`       | OTP ép 110 (TR8)           | 344 ms         | Sạch                               |
| `lut15f50`     | TR8 nạp qua thanh ghi      | 343 ms         | Sạch                               |
| `lut15f65`     | 15 khung, 65 Hz             | 266 ms         | Sạch                               |
| `lut12f50`     | 12 khung, 50 Hz             | 283 ms         | Xám rất nhẹ                      |
| `lut10f50`     | 10 khung, 50 Hz             | 243 ms         | Xám nhẹ                           |
| `lut15f100`    | 15 khung, 100 Hz            | 172 ms         | Thấy bóng ô cờ khi soi kỹ      |
| `lut6f50`      | 6 khung, 50 Hz              | 163 ms         | Xám rõ, không thấy ô cờ       |
| `lut4f200`     | 4 khung, 200 Hz             | 31 ms          | Chưa đo                           |
| `3x5f50`       | 3 lần quét 5 khung, 50 Hz | Khoảng 430 ms | Sạch                               |

Đổi profile theo hai đường:


| Đường                                    | Lệnh console                                                    | Nằm ở                                    |
| --------------------------------------------- | ------------------------------------------------------------------ | -------------------------------------------- |
| Chọn trong danh sách, qua interface chung | `display profile list`, `display profile set <slot> <tên>`      | `display`, không biết tên chip          |
| Chỉnh tay từng thông số của LUT        | `epd tune <slot> <khung> <Hz> [giữ] [lần quét] [giữ nguồn]` | `bsp`, gọi hàm riêng của driver UC8253 |

Tốc độ SPI nằm trong config của driver, mặc định 4 MHz.

Driver dựng bảng LUT bằng khối C thuần `waveform` (có host test): điểm đổi màu kéo đủ số khung, điểm giữ màu đứng yên
rồi kéo lại số khung của xung giữ. Profile có số khung 0 thì driver chép nguyên bảng TR8 đọc từ OTP. Điện áp, VCOM và
tuỳ chọn LUT luôn lấy từ dải TR8 trong OTP, đọc một lần ở lần đầu nạp LUT.

### Kiểm tra cân bằng điện tích

Một điểm đi trắng → đen → trắng chạy qua bảng trắng → đen rồi bảng đen → trắng. Nếu tổng (mức điện áp × số khung) của
hai bảng khác 0 (VSH tính +1, VSL tính −1, 0 V tính 0), điện tích tích dần trong lớp mực, hạt mực bị kẹt và để lại bóng
lâu dài.

- Driver tính tổng này mỗi khi dựng LUT từ profile hoặc từ `epd tune`. Tổng khác 0 thì `epd tune` từ chối, còn profile
  chỉ được nạp khi có cờ `allow_unbalanced`.
- Bảng giữ màu có tổng khác 0 thì driver chỉ cảnh báo. Bảng TR8 cũng vậy: bảng đen giữ đen có 1 khung VSH.
- Bảng sóng OTP do hãng viết nên không kiểm.

## Hiện tượng vật lý của mực và tầng xử lý


| Hiện tượng                                                                            | Tầng xử lý                                                                                |
| ------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------- |
| Điện trường của điểm đang đẩy lan sang điểm bên cạnh, để lại viền xám | `display_policy` đếm partial rồi full; profile có xung giữ; `display` đẩy lại viền  |
| Mất cân bằng điện tích làm hạt mực bị kẹt                                     | Driver kiểm bảng sóng                                                                     |
| Hạt chưa tới đích, màu xám                                                        | Profile (số khung, tần số khung); `display_policy` full refresh                           |
| Dòng rò transistor TFT làm sụt điện áp trong một khung                           | Phần cứng; phần mềm chỉ chỉnh tần số khung                                           |
| Nhiệt độ đổi độ nhớt của mực                                                   | Driver đọc cảm biến; `display` chọn profile theo bảng dải nhiệt độ của từng slot |
| Bóng kéo đuôi khi hình chuyển động                                               | `display` và app                                                                            |

**Bảng dải nhiệt độ.** `display` đọc nhiệt độ trước mỗi lần full và chọn profile cho từng slot theo bảng dải. Bảng
có một dải 0–40 °C (dải nhiệt độ hoạt động của panel), mỗi slot một profile; `display profile set` đổi profile của
dải đó. Lệnh `display temp` in nhiệt độ controller đo được.

**Đẩy lại viền.** Policy nới vùng đổi thêm 1–2 điểm. Ở các điểm viền không đổi màu, `display` đảo bit cũ, nên controller
coi đó là một lần đổi màu và đẩy điểm về đúng màu của nó. Bật tắt bằng lệnh console `display edge on [số điểm]` và `display edge off`, mặc định tắt.

## Đo

- Mỗi lần vẽ ghi thời điểm 6 mốc ở bảng "Một lần vẽ đi qua đâu". Thời điểm bấm do task main đưa vào lúc giao khung;
  các mốc khác `display` tự ghi: mượn khung, giao khung, task màn lấy khung, so xong, và các số thời gian driver trả về.
- Mỗi lần vẽ in một dòng log, ví dụ `flip 362ms partial: input 12 draw 8 queue 1 diff 2 spi 25 panel 327`. `input` là từ
  lúc bấm tới lúc mượn khung, `queue` là từ lúc giao khung tới lúc task màn lấy khung.
- Lệnh console `display stats`: trung bình, nhỏ nhất, lớn nhất từng đoạn trên 50 lần vẽ gần nhất, kèm số khung bị
  bỏ. Lệnh chạy trên task REPL nên chỉ báo cho task màn; task màn giữ số liệu và tự in.

## Khi nào xem lại

Nếu đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng) thì bàn chuyển sang gửi danh sách lệnh vẽ để task màn dựng ảnh.
