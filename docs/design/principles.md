# Nguyên tắc thiết kế

Luật chung, áp dụng cho mọi module.

## 1. Core module thuần, shell module mỏng

**Vấn đề:** logic khó (wake session, gesture, chấm điểm, policy vẽ màn) mà dính FreeRTOS hay phần cứng thì chỉ test
được trên mạch, và lỗi khó tái hiện.

**Cách xử lý:** phần quyết định viết thành core module bằng C thuần: nhận event, trả action. Phần đụng tới task,
queue, driver là shell module mỏng bọc bên ngoài.

| Core module | Quyết định gì | Shell module |
|---|---|---|
| `session` | State machine của wake session: wake → active → save → deep sleep | `system`: main queue, gọi driver nguồn |
| `gesture` | Edge của nút, IMU → gesture | `input`: interrupt GPIO, timer |
| `keymap` | Gesture → command theo keymap table | `system`: giữ và lưu keymap table |
| `study` | Hàng thẻ đến hạn, chấm điểm | Port nối storage và đồng hồ |
| `display_policy`, `frame_slots` | So frame, debt và edge flag của từng pixel, chọn decision, cleanup stage; owner của frame | `display`: display task, gọi driver, timer cleanup |

## 2. State thuộc module nào thì chỉ module đó sửa

**Vấn đề:** chỉ có main task giữ state nên không cần lock, nhưng dễ trượt thành biến toàn cục mà hàm nào cũng sửa.

**Cách xử lý:** mỗi module giữ state của mình (biến `static`). Module khác muốn đọc hay đổi thì gọi hàm của module đó.

## 3. App chỉ gọi đúng thứ nó cần

App được gọi `system`, `study`, `display`, `ui`. App không gọi thẳng `storage` hay `hal`; dữ liệu học đi qua `study`.

## 4. Một nguồn sự thật cho mỗi thông tin

| Thông tin | Nguồn chính | Nơi khác |
|---|---|---|
| Chân GPIO firmware dùng | Pin map trong `bsp/` | `docs/hardware/boards/` ghi cách đấu dây thật của board; hai bên phải khớp |
| Định dạng bản ghi chấm điểm | Bộ test vector | Docs giải thích; mọi bản code (máy, điện thoại, Kindle) phải chạy qua test vector |
| Thông số linh kiện | `docs/hardware/` | Code không chép lại |

Không gộp hai đoạn code chỉ vì trông giống nhau, nếu chúng sẽ đổi vì những lý do khác nhau.

## 5. Chỉ dựng phần bản hiện tại dùng tới

**Vấn đề:** thiết kế trước cho tính năng tương lai (nhiều app, đồng bộ, giọng nói) dễ sinh code không ai dùng mà vẫn
phải bảo trì.

**Cách xử lý:** chỉ viết interface, struct và module mà bản hiện tại dùng. Với tính năng tương lai, chỉ cần không chặn đường thêm vào
sau. Capabilities của driver chỉ khai báo khả năng đang được dùng.
