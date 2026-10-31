# Module e-paper WeAct 3.7": điện

Nguồn cấp, mức logic, dòng tiêu thụ và các giới hạn môi trường. Số điện áp cao (±15 V, ±20 V) do module tự tạo, không phải cấp từ ngoài.

## Ví dụ

Cấp 3,3 V từ DevKit vào chân VCC và để module ở cấu hình 3,3 V. Mỗi lần làm mới màn tốn khoảng 200 mA·s, tức khoảng 0,056 mAh (200 chia 3600). Màn nằm yên giữa hai lần làm mới thì gần như không tốn điện; controller ở deep sleep tốn tối đa 3 µA.

## Nguồn cấp module


| Hạng mục                                  | Giá trị                                                        | Nguồn                         | Trạng thái    |
| --------------------------------------------- | ------------------------------------------------------------------ | -------------------------------- | ----------------- |
| Điện áp vào chân VCC, hàn SB4 + SB5   | 3,3–5 V; đi qua LDO ME6231A33M3G, ra 3,3 V                     | Bản vẽ hình dạng; sơ đồ | Đã xác nhận |
| Điện áp vào chân VCC, hàn SB3         | Chỉ 3,3 V; bỏ qua LDO, VCC nối thẳng vào rail 3,3 V         | Bản vẽ hình dạng; sơ đồ | Đã xác nhận |
| Mức logic I/O                              | 3,3 V                                                            | Bản vẽ hình dạng           | Đã xác nhận |
| Cầu hàn nào đang hàn sẵn trên module | SB1,2,4,5                                                        | Người dùng                  | Đã xác nhận |
| Không hàn SB3 cùng lúc SB4/SB5          | Hai đường cùng nối vào rail 3,3 V (suy luận từ sơ đồ) | Sơ đồ                       | Đã xác nhận |

## LDO ME6231A33M3G (trên module)


| Thông số           | Giá trị               | Nguồn      |
| ---------------------- | ------------------------- | ------------- |
| Điện áp ra        | 3,3 V, chính xác ±1% | LDO tr.1–2 |
| Dòng ra tối đa    | 500 mA (Vin = 4,3 V)    | LDO tr.1    |
| Điện áp rơi      | 125 mV tại 100 mA      | LDO tr.1    |
| Dải điện áp vào | 3–18 V                 | LDO tr.1    |
| Dòng tự tiêu thụ | 1,8 µA (điển hình)  | LDO tr.1    |

## Panel và controller


| Thông số                        | Giá trị                                                             | Nguồn       | Trạng thái    |
| ----------------------------------- | ----------------------------------------------------------------------- | -------------- | ----------------- |
| VCC panel                         | 2,4 / 3,0 / 3,6 V (nhỏ nhất / điển hình / lớn nhất)            | Panel tr.11  | Đã xác nhận |
| VDD, VDDIO của UC8253            | 2,3 / 3,3 / 3,6 V                                                     | UC8253 tr.59 | Đã xác nhận |
| Dòng khi đang làm mới         | Tối đa 8 mA                                                         | Panel tr.11  | Đã xác nhận |
| Dòng deep sleep                  | Tối đa 3 µA tại VCC = 3,0 V                                       | Panel tr.11  | Đã xác nhận |
| Năng lượng một lần làm mới | 200 mA·s điển hình ở 25 °C (dòng trung bình nhân thời gian) | Panel tr.11  | Đã xác nhận |
| Mức vào cao tối thiểu         | 0,8 × VDDIO                                                          | Panel tr.11  | Đã xác nhận |
| Mức vào thấp tối đa          | 0,2 × VDDIO                                                          | Panel tr.11  | Đã xác nhận |

Chân BUSY ra mức cao tối thiểu 0,9 × VDDIO và mức thấp tối đa 0,1 × VDDIO ở dòng 100 µA (Panel tr.11).

## Điện áp cao do module tạo

Controller có bộ tăng áp bên trong, cộng với mạch ngoài (cuộn 47 µH, MOSFET AO3402, ba diode B5819W, tụ 1 µF) nâng 3,3 V lên các mức dưới đây.


| Rail                                         | Giá trị mặc định              | Cách chỉnh                                      | Nguồn           |
| ---------------------------------------------- | ------------------------------------ | --------------------------------------------------- | ------------------ |
| VGH (điện áp cực cổng, bật TFT)        | +20 V                              | Lệnh `0x01` byte 2, từ ±20 V xuống ±10 V     | UC8253 tr.16     |
| VGL (điện áp cực cổng, tắt TFT)        | −20 V                             | Cùng lệnh                                       | UC8253 tr.16     |
| VSH (đẩy điểm ảnh đen/trắng, dương) | +15 V (mặc định thanh ghi `3F`) | Lệnh `0x01`, từ +2,4 V tới +15 V, bước 0,2 V | UC8253 tr.16–17 |
| VSL (đẩy điểm ảnh, âm)                 | −15 V (mặc định)               | Lệnh `0x01`, từ −2,4 V tới −15 V             | UC8253 tr.16–17 |
| VDHR (điểm ảnh đỏ)                      | +5,0 V (mặc định `0D`)          | Lệnh `0x01`; module đen trắng không dùng     | UC8253 tr.16–18 |

Các mức này còn được chương trình sóng (LUT) lưu trong OTP đặt lại theo từng dải nhiệt độ (UC8253 tr.52); xem `waveform.md`.

## Giới hạn môi trường và tuổi thọ


| Thông số                             | Giá trị                                                                          | Nguồn                    |
| ---------------------------------------- | ------------------------------------------------------------------------------------ | --------------------------- |
| Điện áp logic tuyệt đối tối đa | −0,5 tới 6,0 V (panel); −0,3 tới 6,0 V (UC8253)                                | Panel tr.11; UC8253 tr.58 |
| Nhiệt độ hoạt động               | 0–40 °C                                                                          | Panel tr.11               |
| Nhiệt độ lưu kho và vận chuyển  | −25–60 °C                                                                       | Panel tr.11               |
| Độ ẩm khuyến nghị                 | 45–70 %RH; ngoài khoảng này không đảm bảo chất lượng 5 năm             | Panel tr.11–12           |
| Trên 35 °C                           | Hiệu ứng điểm đơn không được đảm bảo; nên làm mới mỗi 24 giờ     | Panel tr.11               |
| Cất kho quá 10 ngày ở 40–60 °C   | Chất lượng hiển thị có thể giảm                                            | Panel tr.11               |
| Cảm biến nhiệt trong controller     | −25–50 °C, sai số ±2 °C                                                      | UC8253 tr.3               |
| Tuổi thọ thiết kế                  | 5 năm ở 25 °C/60 %RH                                                            | Panel tr.16               |
| ESD (machine model)                    | ±250 V                                                                            | Panel tr.16               |
| Bảo hành                             | 12 tháng cộng 1 tháng giao hàng                                                | Panel tr.16               |
| Độ tương phản                     | Trạng thái đen L* ≤ 20, trạng thái trắng L* ≥ 66, ΔE bóng mờ tối đa 2 | Panel tr.12               |

Panel tr.12 còn dặn: nên làm mới ít nhất một lần mỗi ngày; điều này khớp với lời khuyên làm mới định kỳ ở `refresh.md`.

Đối chiếu gốc: `source/panel-e037a75-a0-spec.pdf` tr.10–12, 16; `source/uc8253-datasheet.pdf` tr.3, 16–18, 58–59; `source/ldo-me6231a33m3g-datasheet.pdf` tr.1–2.
