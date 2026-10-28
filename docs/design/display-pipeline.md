# Đường vẽ màn

## Vấn đề

Lật thẻ phải nhanh (≤ 0,3 s rồi giảm dần), và muốn tối ưu thì phải biết thời gian đi vào đâu. Gần hết thời gian là
lúc panel quét (~350 ms theo thư viện GxEPD2, chưa đo), nằm trong driver màn.

## Một lần vẽ đi qua đâu

| # | Đoạn                                                                          | Ai làm                 | Task         |
| - | ------------------------------------------------------------------------------- | ----------------------- | ------------ |
| 1 | Bấm → cử chỉ → intent                                                      | `gesture`, `keymap` | main         |
| 2 | App vẽ vào canvas (ảnh 1 bit cả màn)                                       | app + `ui`            | main         |
| 3 | Giao khung cho task màn qua vùng đệm "khung mới nhất" (3 khung, RAM nội) | `display`             | main → màn |
| 4 | So với khung cũ, tìm vùng đổi, chọn partial/full                         | `display`             | màn         |
| 5 | Gửi ảnh qua SPI                                                               | driver                  | màn         |
| 6 | Panel quét, chờ chân BUSY                                                    | driver                  | màn         |

Bấm nhanh hơn tốc độ màn thì các khung giữa bị bỏ, chỉ khung mới nhất được vẽ.

## Giao khung giữa task main và task màn

Ví dụ: app lật thẻ. Task main mượn một khung, khung này đã có sẵn ảnh của lần giao trước, nên app chỉ cần vẽ phần
đổi. Vẽ xong, task main giao khung kèm thời điểm bấm nút. Task màn thức dậy, lấy khung đó ra vẽ.

Vùng đệm có 3 khung trong RAM nội. Lúc nào mỗi khung cũng có đúng một chủ:

| Chủ | Khung |
| --- | ----- |
| Task main | Khung đang vẽ (đã mượn, chưa giao) |
| Hàng chờ | Khung mới nhất đã giao, task màn chưa lấy |
| Task màn | Khung đang vẽ ra màn, sau đó là khung đang hiện |

- Task màn trả khung đang hiện ngay khi so xong với khung mới, vì driver tự giữ ảnh cũ trong controller. Nhờ vậy
  task main luôn mượn được khung, không bao giờ phải chờ. Nếu main mượn đúng lúc task màn đang so (giữ cả hai
  khung), main lấy lại khung trong hàng chờ; khung đó cũng tính là bị bỏ.
- Giao khung khi hàng chờ đã có khung thì khung cũ trong hàng chờ bị bỏ và được đếm.
- Mượn lần hai khi chưa giao khung đã mượn là lỗi của bên gọi.

## Chọn cách vẽ

Phần quyết định nằm trong khối C thuần `display_policy`, có host test. Nó so khung mới với khung đang hiện, lấy hình
chữ nhật bao vùng đổi (mép x căn theo bước căn của driver), rồi chọn theo thứ tự, gặp điều kiện nào trước thì dừng:

| # | Điều kiện | Cách vẽ |
| - | --------- | ------- |
| 1 | Lần vẽ đầu, hoặc lần vẽ trước bị lỗi | Full |
| 2 | Khung giống hệt khung đang hiện | Không vẽ |
| 3 | Driver không có partial | Full |
| 4 | Số lần partial liên tiếp đã tới ngưỡng bóng mờ | Full, để làm sạch |
| 5 | Vùng đổi lớn hơn ngưỡng % diện tích màn | Full |
| 6 | Còn lại | Partial, chỉ vùng đổi |

Ngưỡng nằm trong struct cấu hình. Mặc định: bóng mờ 10 lần partial; vùng lớn 100 %, tức là tắt luật số 5, vì một
lần full mất khoảng 2 s còn partial vùng lớn vẫn nhanh hơn. Hai số này sẽ chỉnh theo kết quả đo.

## Chia việc giữa display và driver

| Việc                                                                                                   | Ai lo                                                                    |
| ------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| Waveform/LUT, tốc độ SPI, nguồn analog của panel, chế độ điều khiển từng điểm ảnh        | Driver                                                                   |
| Nạp "ảnh cũ" vào controller mà không làm mới màn (dùng khi thức dậy để màn không chớp) | Driver cung cấp, `display` gọi                                       |
| So khung, bỏ khung giữa, chọn partial/full, đếm bóng mờ để thỉnh thoảng làm sạch           | `display`, phần quyết định viết bằng C thuần để test trên PC |

Driver khai báo nó làm được gì qua bảng khả năng: cỡ màn, có partial refresh không, có làm sạch sâu không, thời gian
điển hình. `display` dựa vào đó để chọn cách vẽ, không hỏi tên chip.

## Hợp đồng driver màn

Ví dụ: `display` muốn đổi một ô chữ ở góc trên. Nó gọi hàm vẽ của driver với con trỏ tới cả khung mới, vùng cần đổi
(x 0–79, y 0–39) và kiểu partial. Driver tự cắt 40 hàng × 10 byte của vùng đó, gửi qua SPI, cho panel quét, chờ BUSY
lên cao rồi trả về thời gian SPI và thời gian panel.

- **Đồng bộ:** hàm vẽ gửi ảnh rồi ngủ chờ ngắt BUSY, xong mới trả về. Chỉ task màn gọi nên không chặn ai khác.
- Driver báo lại thời gian đoạn 5 và đoạn 6 sau mỗi lần vẽ.

| Điều khoản | Quy định |
| ---------- | -------- |
| Toạ độ | Theo hướng gốc của panel (với màn 3,7" là 240 × 416, dọc). Xoay màn do tầng trên lo. |
| Khung ảnh | 1 bit mỗi điểm, bit 1 = trắng, bit 0 = đen. Gói theo hàng, điểm trái nhất ở bit cao nhất của byte. |
| Ảnh đưa vào | Luôn là con trỏ tới cả khung kèm vùng cần vẽ; driver tự cắt vùng, `display` không phải chép ảnh. |
| Kiểu vẽ | Full (cả màn, sạch, chậm) hoặc partial (chỉ vùng đã cho, nhanh). |
| Mép vùng partial | Driver làm tròn ra ngoài theo bước căn mép x mà nó khai báo trong bảng khả năng. |
| Ảnh cũ trong controller | Driver tự giữ cho khớp với ảnh đang hiện: sau khi panel quét xong, nó ghi lại vùng vừa vẽ vào vùng ảnh cũ. Việc này nằm ngoài đường đo bấm → BUSY lên cao. |
| Thời gian trả về | Ba số: SPI trước khi quét (đoạn 5), panel quét tới khi BUSY lên cao (đoạn 6), việc sau khi quét (tắt nguồn analog, ghi lại ảnh cũ). |
| Giữa các lần vẽ | Controller tắt nguồn analog nhưng giữ thanh ghi và RAM ảnh, không ngủ sâu. |

Bảng khả năng của driver màn:

| Mục | Nghĩa |
| --- | ----- |
| Cỡ gốc | Rộng và cao theo hướng gốc của panel |
| Có partial | Driver vẽ được kiểu partial |
| Bước căn mép x | Mép trái và mép phải vùng partial phải là bội của số này |
| Thời gian điển hình | Một lần full và một lần partial, để `display` ước lượng |

## Đo

- Mỗi lần vẽ ghi thời điểm 6 mốc ở bảng trên. Thời điểm bấm do task main đưa vào lúc giao khung; các mốc khác
  `display` tự ghi: mượn khung, giao khung, task màn lấy khung, so xong, và hai số thời gian driver trả về.
- Mỗi lần vẽ in một dòng log, ví dụ `flip 362ms partial: input 12 draw 8 queue 1 diff 2 spi 25 panel 327`. `input` là từ
  lúc bấm tới lúc mượn khung, `queue` là từ lúc giao khung tới lúc task màn lấy khung.
- Lệnh console `display stats`: trung bình, nhỏ nhất, lớn nhất từng đoạn trên 50 lần vẽ gần nhất, kèm số khung bị
  bỏ. Lệnh chạy trên task REPL nên chỉ báo cho task màn; task màn giữ số liệu và tự in.

## Khi nào xem lại

Nếu đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng) thì bàn chuyển sang gửi danh sách lệnh vẽ để task màn dựng ảnh.
