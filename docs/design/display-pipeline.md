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

- **Đồng bộ:** hàm vẽ gửi ảnh rồi ngủ chờ ngắt BUSY, xong mới trả về. Chỉ task màn gọi nên không chặn ai khác.
- Driver báo lại thời gian đoạn 5 và đoạn 6 sau mỗi lần vẽ.

## Đo

- Mỗi lần vẽ ghi thời điểm 6 mốc ở bảng trên.
- Mỗi lần vẽ in một dòng log, ví dụ `flip 362ms: draw 8 diff 2 spi 25 panel 327`.
- Lệnh console `display stats`: trung bình, nhỏ nhất, lớn nhất từng đoạn trên 50 lần vẽ gần nhất.

## Khi nào xem lại

Nếu đoạn 2 vượt 50 ms (ví dụ dựng chữ, ảnh, dithering nặng) thì bàn chuyển sang gửi danh sách lệnh vẽ để task màn dựng ảnh.
