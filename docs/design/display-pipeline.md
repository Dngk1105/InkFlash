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

- Mỗi lần vẽ ghi thời điểm 6 mốc ở bảng trên.
- Mỗi lần vẽ in một dòng log, ví dụ `flip 362ms: draw 8 diff 2 spi 25 panel 327`.
- Lệnh console `display stats`: trung bình, nhỏ nhất, lớn nhất từng đoạn trên 50 lần vẽ gần nhất.

## Khi nào xem lại

Nếu đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng) thì bàn chuyển sang gửi danh sách lệnh vẽ để task màn dựng ảnh.
