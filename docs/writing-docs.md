# Viết docs

## Ví dụ

| Không | Có |
|---|---|
| Task màn luôn vẽ khung mới nhất, bỏ khung giữa. | Display task luôn vẽ frame mới nhất. Frame pending bị drop khi main task submit frame mới. |
| Chờ panel quét một lúc rồi mới tắt nguồn. | Display task tắt analog power sau `power_off_ms` không có DRF. |
| Về cơ bản policy sẽ dọn màn khi cần thiết. | Frame có intent PAGE và debt lớn nhất ≥ `debt_page`: policy chọn cleanup strong. |

## Ngôn ngữ

- Tên file tiếng Anh. Nội dung viết câu tiếng Việt.
- Thuật ngữ viết đúng `term` trong `docs/glossary.yaml`, không dịch. `python tools/terms.py check` báo lỗi khi gặp từ trong `avoid`.
- Thuật ngữ chưa có trong glossary: thêm vào glossary trước, rồi mới dùng.
- Tên hàm, struct, file, lệnh đặt trong backtick: `ink_display_submit()`.

## 5C

| Quy tắc | Nghĩa | Không | Có |
|---|---|---|---|
| Clear | Câu đơn, một nghĩa | "Chủ của 3 khung đệm của display" | "`ink_frame_slots` giữ owner của 3 frame." |
| Concise | Bỏ chữ thừa, đi thẳng vào việc | "Hàng chờ đã có khung thì khung cũ đó bị bỏ" | "`pending` đã có frame: frame cũ bị drop." |
| Complete | Đủ bước, thông số, điều kiện | "Driver chờ BUSY." | "Driver chờ BUSY lên; timeout `drf_ms × 1,5 + 100 ms`." |
| Correct | Tên linh kiện, part number, đơn vị, số liệu đã kiểm | Số không rõ nguồn | Số lấy từ code, `docs/hardware/` hoặc datasheet. Chưa có thì hỏi người làm |
| Concrete | Định tính đổi thành định lượng | "SPI chạy nhanh." | "SPI chạy 4 MHz khi `spi_clock_hz = 0`." |

## Giọng văn

- Mẫu câu: `[chủ thể] [làm gì] [vì sao/khi nào]`.
- Mỗi đoạn một ý. Bước làm thì đánh số, mỗi bước một hành động.
- Bỏ từ đệm: "về cơ bản", "thực ra", "có lẽ", "thường thì", "khá", "một chút".
- Bỏ đại từ thừa ("chúng ta", "bạn") khi chủ thể đã rõ.

## Nội dung

- Docs chỉ mô tả mức hệ thống: các khối, luồng, ranh giới, lý do, ràng buộc phần cứng. Thuật toán và chi tiết triển khai đọc từ code, không chép vào docs.
- Docs đi trước code: thiết kế đã chốt thì ghi vào docs trước, rồi mới viết code theo docs. Docs không được sai so với code.
- Docs chỉ ghi điều đang đúng. Việc còn thiếu, lỗi đã biết, điểm mở rộng ghi bằng tag trong code (`docs/writing-comments.md`, mục "Tag việc dở").
- Ít code, câu dễ hiểu.
- Quan hệ giữa các khối viết bằng bảng, mỗi quan hệ một ô. Không vẽ sơ đồ bằng ký tự.
- Yêu cầu chất lượng chỉ ghi khi có test đo nó, kèm kết quả đo.
- Kết thúc một phiên bản: ghi những gì đã làm được, gắn tag git.

## Cấu trúc thư mục

- File trong `docs/design/` theo khung: Vấn đề → Cách xử lý. Mỗi file một vấn đề, tự đứng được: chép lại vài câu ngữ cảnh cần thiết thay vì link sang file thiết kế khác. Link chỉ dùng trong `architecture.md`.
- Thêm file thiết kế mới thì thêm dòng vào bảng "Gặp vấn đề" trong `architecture.md`.
- Quyết định mới: ghi vào file thiết kế của vấn đề đó; vấn đề mới thì tạo file mới. Lịch sử thay đổi xem bằng git.
- Phần cứng: mỗi linh kiện một thư mục `docs/hardware/parts/<linh-kiện>/` (bắt buộc có `overview.md`, mỗi đặc tính thêm một file riêng); mỗi board một file `docs/hardware/boards/<mã-board>.md`. Thêm linh kiện, file hoặc board thì sửa bảng trong `docs/hardware/index.md`.
