# Viết comment trong code

Ví dụ:


| Comment                                                                      | Giữ hay bỏ                                           | Vì sao                                                                            |
| ------------------------------------------------------------------------------ | -------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| `/* Enum chỉ thêm giá trị vào cuối: bảng gán đã lưu dạng số */` | Giữ                                                   | Ràng buộc đến từ nơi khác, đọc code không thấy                          |
| `/* Trả ERROR_CONFIG và giữ config cũ */` trên hàm public                | Giữ                                                   | Hợp đồng với bên gọi                                                         |
| `bool stable; /* level sau debounce; true = bấm */`                         | Giữ                                                   | Tên chưa nói hết nghĩa của biến                                             |
| `/* Config đúng khi debounce_ms < double_ms, ... */`                       | Bỏ                                                    | Chép lại điều kiện trong code: code đổi thì comment sai mà không ai báo |
| `/* Mask đổi giữa chừng thì bỏ cử chỉ dở */`                        | Bỏ                                                    | Hành vi đã có trong `docs/design/` và host test                               |
| `/* IMU, giọng nói thêm vào đây */`                                    | Bỏ                                                    | Kế hoạch, không phải điều đang đúng                                       |
| `/* đã nhả, chờ hết double_ms xem có lần bấm thứ hai */`            | Viết gọn: `/* đã nhả, chờ lần bấm thứ hai */` | `double_ms` đã có ngay trong code dùng nó                                     |

Luật:

- Comment chia file code thành từng khối, phần theo chức năng nghiệp vụ, đảm bảo các code liên quan gần nhau trong
  một khu vực. Mẫu: `/* ---- Hàng đợi task main ---- */`, dòng tiếp theo (nếu cần) ghi khối đó làm gì.
- Comment chỉ ghi điều code không tự nói được: vì sao, ràng buộc, hợp đồng với bên gọi, nghĩa của biến khi tên
  chưa đủ.
- Không chép lại hành vi. Hành vi mức hệ thống nằm trong `docs/design/`; hành vi chi tiết nằm trong host test, vì
  test tự báo lỗi khi logic đổi còn comment thì không.
- Hàm public trong header: ghi khi nào trả lỗi gì, ai gọi và gọi lúc nào. Không liệt kê điều kiện kiểm tra bên trong.
- Không ghi kế hoạch hay tính năng chưa có.
- Viết cụm từ ngắn, không cần câu đầy đủ. Bỏ phần tên biến hoặc code đã nói.
- Thuật ngữ kỹ thuật giữ tiếng Anh: edge, debounce, deadline, timer, callback, gesture, modifier, mask, state
  machine, press, hold, config,... Thống nhất chung một cách gọi xuyên suốt (nếu dùng tiếng anh thì không dùng tiếng việt nữa)
- Dòng đầu mỗi file là `Thiết kế: docs/design/<file>.md`; đó là chỗ nối code với docs, không cần chép docs vào comment.
- Đổi code thì sửa comment trong cùng commit. Comment sai so với code coi như lỗi.
