# Viết docs

- Tên file tiếng Anh, nội dung tiếng Việt.
- Docs chỉ mô tả mức hệ thống: các khối, luồng, ranh giới, lý do, ràng buộc phần cứng. Thuật toán và chi tiết
  triển khai đọc từ code, không chép vào docs.
- Docs đi trước code: thiết kế đã chốt thì ghi vào docs trước, rồi mới viết code theo docs. Docs không được sai so với code.
- Không ghi kế hoạch, lộ trình, điều chưa chốt.
- Ít code, câu dễ hiểu.
- Quan hệ giữa các khối viết bằng bảng (mỗi quan hệ một ô rõ ràng), không vẽ sơ đồ bằng ký tự.
- File trong `docs/design/` theo khung: Vấn đề → Cách xử lý. Mỗi file một vấn đề, tự đứng được: chép lại vài câu
  ngữ cảnh cần thiết thay vì link sang file thiết kế khác. Link chỉ dùng trong `architecture.md`.
- Thêm file thiết kế mới thì thêm dòng vào bảng "Gặp vấn đề" trong `architecture.md`.
- Phần cứng: mỗi linh kiện một thư mục `docs/hardware/parts/<linh-kiện>/` (bắt buộc có `overview.md`, mỗi đặc tính
  thêm một file riêng); mỗi board một file `docs/hardware/boards/<mã-board>.md`. Thêm linh kiện, file hoặc board
  thì sửa bảng trong `docs/hardware/index.md`.
- Quyết định mới: ghi vào file thiết kế của vấn đề đó; vấn đề mới thì tạo file mới. Lịch sử thay đổi xem bằng git.
- Yêu cầu chất lượng chỉ ghi khi có test đo nó, kèm kết quả đo.
- Kết thúc một phiên bản: ghi những gì đã làm được, gắn tag git.
