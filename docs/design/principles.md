# Nguyên tắc thiết kế

Luật chung, áp dụng cho mọi khối code.

## 1. Lõi thuần, vỏ mỏng

**Vấn đề:** logic khó (phiên thức, cử chỉ, chấm điểm, chính sách làm mới màn) mà dính với FreeRTOS hay phần cứng
thì chỉ test được trên mạch, và lỗi khó tái hiện.

**Cách xử lý:** phần quyết định viết bằng C thuần: nhận sự kiện, trả về hành động. Phần đụng tới task, hàng đợi,
driver là vỏ mỏng bọc bên ngoài.

| Khối | Phần C thuần | Phần vỏ |
|---|---|---|
| `session/` | Máy trạng thái phiên thức: thức → hoạt động → lưu → ngủ | `system/`: hàng đợi sự kiện, gọi driver nguồn |
| `gesture/` | Nút, IMU → cử chỉ | Ngắt GPIO, hẹn giờ |
| `keymap/` | Cử chỉ → intent theo bảng gán | `system`: giữ và lưu bảng gán |
| `study/` | Hàng thẻ đến hạn, chấm điểm | Cổng nối storage và đồng hồ |
| `display/` | So khung, chọn partial/full, đếm bóng mờ | Task màn, gọi driver |

## 2. Trạng thái thuộc khối nào thì chỉ khối đó sửa

**Vấn đề:** chỉ có một task chính giữ trạng thái nên không cần khoá, nhưng dễ trượt thành biến toàn cục mà hàm nào
cũng sửa.

**Cách xử lý:** mỗi khối giữ trạng thái của mình (biến `static`). Khối khác muốn đọc hay đổi thì gọi hàm của khối đó.

## 3. App chỉ gọi đúng thứ nó cần

App được gọi `system`, `study`, `display`, `ui`. App không gọi thẳng `storage` hay `hal`; dữ liệu học đi qua `study`.

## 4. Một nguồn sự thật cho mỗi thông tin

| Thông tin | Nguồn chính | Nơi khác |
|---|---|---|
| Chân GPIO firmware dùng | Bảng chân trong `bsp/` | `docs/hardware/boards/` ghi cách đấu dây thật của board; hai bên phải khớp |
| Định dạng bản ghi chấm điểm | Bộ test vector | Docs giải thích; mọi bản code (máy, điện thoại, Kindle) phải chạy qua test vector |
| Thông số linh kiện | `docs/hardware/` | Code không chép lại |

Không gộp hai đoạn code chỉ vì trông giống nhau, nếu chúng sẽ đổi vì những lý do khác nhau.

## 5. Chỉ dựng phần khung bản hiện tại dùng tới

**Vấn đề:** thiết kế trước cho tính năng tương lai (nhiều app, đồng bộ, giọng nói) dễ sinh code không ai dùng mà vẫn
phải bảo trì.

**Cách xử lý:** chỉ viết phần khung mà bản hiện tại dùng. Với tính năng tương lai, chỉ cần đảm bảo không chặn đường
thêm vào sau. Bảng khả năng của driver chỉ khai báo khả năng đang được dùng.
