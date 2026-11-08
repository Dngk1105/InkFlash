# Viết comment trong code

## Ví dụ

```c
/* Design: docs/design/display/display.md */

/**
 * @brief Take the pending frame for drawing.
 * @par Called from
 * Display task.
 * @return Frame index, or INK_FRAME_SLOTS_NONE if nothing is pending or state is NULL.
 */
int8_t ink_frame_slots_take(ink_frame_slots_state_t *state);

/**
 * @brief Draw the window from the (new, old) bit pair of each pixel; wait until the panel finishes.
 * @par Called from
 * Display task.
 * @param window x edges are rounded out to a multiple of x_align.
 * @retval ESP_ERR_INVALID_ARG profile is out of range.
 * @retval ESP_ERR_TIMEOUT BUSY stays low longer than drf_ms × 1.5 + 100 ms.
 */
esp_err_t (*draw)(...);

typedef struct {
    uint16_t width;   /**< native panel width, px */
    uint8_t  x_align; /**< window x edges are multiples of this */
} ink_hal_display_capabilities_t;

/* WARN: values stored in NVS by index; append only. */
/* TODO(display): partial draw for windows narrower than 8 px. */
```

## Luật chung

- Comment viết **tiếng Anh**. `python tools/terms.py check` báo lỗi khi comment có chữ tiếng Việt.
- Dùng đúng `term` trong `docs/glossary.yaml`. Ví dụ: viết `display task`, không viết `screen task`. Thuật ngữ chưa có thì thêm vào glossary trước.
- Comment chỉ ghi điều code không tự nói được: vì sao, ràng buộc, hợp đồng với caller, nghĩa của biến khi tên chưa đủ.
- Không chép lại hành vi. Hành vi mức hệ thống nằm trong `docs/design/`. Hành vi chi tiết nằm trong host test, vì test báo lỗi khi logic đổi còn comment thì không.
- Số đi kèm đơn vị: `100 ms`, `4 MHz`, `240 × 416 px`. Không viết "a while", "fast", "large".
- Câu ngắn, bỏ phần tên biến đã nói.
- Dòng đầu mỗi file: `/* Design: docs/design/<file>.md */`. Đây là chỗ nối code với docs; không chép docs vào comment.
- Chia file thành khối theo chức năng: `/* ---- Main task queue ---- */`. Dòng sau (nếu cần) ghi khối đó làm gì.
- Đổi code thì sửa comment trong cùng commit. Comment sai so với code là lỗi.

## Comment tốt và comment thừa

| Comment | Giữ hay bỏ | Vì sao |
|---|---|---|
| `/* Append only: assignments are stored as numbers. */` | Giữ | Ràng buộc đến từ nơi khác, đọc code không thấy |
| `/* Returns ERROR_CONFIG and keeps the old config. */` trên hàm public | Giữ | Hợp đồng với caller |
| `bool stable; /* level after debounce; true = pressed */` | Giữ | Tên chưa nói hết nghĩa |
| `/* Config is valid when debounce_ms < double_ms ... */` | Bỏ | Chép điều kiện trong code; code đổi thì comment sai mà không ai báo |
| `/* Mask changed mid-gesture: drop the gesture. */` | Bỏ | Hành vi đã có trong `docs/design/` và host test |
| `/* released, waiting double_ms for a second press */` | Viết gọn: `/* released, waiting for a second press */` | `double_ms` đã có trong code ngay đó |

## Header public: Doxygen

Hàm, struct, enum trong `include/*.h` dùng `/** */` ngay trên khai báo. clangd đọc comment này để hiện khi hover.

| Thẻ | Khi nào | Nội dung |
|---|---|---|
| `@brief` | Luôn có | Một câu, động từ đứng đầu |
| `@par Called from` | Luôn có với hàm public | Dòng sau: task nào gọi, hoặc gọi lúc nào (`Main task.`, `Boot, before the display task starts.`) |
| `@param` | Khi tên và kiểu chưa đủ | Đơn vị, giới hạn, nghĩa của `NULL` |
| `@retval` | Hàm trả mã lỗi (`esp_err_t`, `*_error_t`) | Mỗi mã lỗi một dòng: mã, rồi điều kiện. Không cần ghi mã OK |
| `@return` | Hàm trả giá trị không phải mã lỗi | Giá trị trả về và giá trị đặc biệt (`NONE`, `NULL`, `0`) |
| `/**< */` cuối dòng | Field, giá trị enum | Chỉ khi tên chưa nói hết nghĩa |

- Hàm `static` và biến trong `.c` dùng `/* */` ngắn, theo luật chung.
- `tools/terms.py check` báo lỗi khi hàm public thiếu `/** */`, thiếu `@brief`, hoặc trả mã lỗi mà thiếu `@retval`.

### Sinh khung comment bằng doxdocgen

Extension `cschlosser.doxdocgen` đã cấu hình trong `.vscode/settings.json`.

1. Đặt con trỏ ở dòng trống ngay trên khai báo hàm.
2. Gõ `/**`, rồi nhấn `Enter`.
3. Điền chữ vào khung vừa sinh. Xoá `@param` khi tên và kiểu đã đủ nghĩa. Thêm một dòng `@retval` cho mỗi mã lỗi.

Khung sinh cho `ink_frame_slots_init(ink_frame_slots_state_t *state)`:

```c
/**
 * @brief
 * @par Called from
 *
 * @param state
 * @retval
 */
```

- doxdocgen tự bỏ qua attribute `[[nodiscard]]` trước kiểu trả về.
- Hàm trả `bool` sinh sẵn `@retval true` và `@retval false`.
- Hàm trả giá trị không phải mã lỗi: đổi `@retval` thành `@return`.

### Xem comment khi hover

clangd (`esp-clangd 21.1.3`) hiện comment `/** */` khi hover vào hàm, struct, enum, field.

- clangd 21 hiện `@brief`, `@param`, `@retval` dạng chữ thô, không định dạng. Tuỳ chọn `Documentation: CommentFormat` không có trong clangd 21 (đã thử: báo `Unknown Config key`). Vì vậy mỗi thẻ viết một dòng.
- Hover thường vào struct không liệt kê field. Giữ `Ctrl` rồi hover vào tên kiểu (ví dụ `ink_frame_slots_state_t`) để xem toàn bộ định nghĩa, gồm mọi field và comment `/**< */`. `Alt+F12` (Peek Definition) mở định nghĩa ngay tại chỗ.

## Tag việc dở

Docs chỉ mô tả kiến trúc hiện tại. Việc còn thiếu, lỗi, chỗ cần duyệt, điểm mở rộng ghi bằng comment có tag, ngay tại
dòng code liên quan. Người và agent cứ gắn tag khi thấy cần; thừa một tag còn hơn quên một việc.

| Tag | Dùng khi | Ví dụ |
|---|---|---|
| `TODO` | Chức năng còn thiếu, đã biết cần làm gì | `/* TODO(display): partial draw for windows narrower than 8 px. */` |
| `FIXME` | Lỗi đã biết nguyên nhân, chưa sửa | `/* FIXME(input): edge lost when both buttons change in the same tick. */` |
| `BUG` | Lỗi đã thấy, chưa rõ nguyên nhân | `/* BUG(drv_uc8253): BUSY stays low ~1/200 draws at 5 °C; cause unknown. */` |
| `HACK` | Cách làm tạm, chạy được nhưng không đúng; ghi cách đúng | `/* HACK(display): fixed 20 ms delay; should wait for the BUSY edge. */` |
| `PERF` | Chỗ chậm hoặc tốn bộ nhớ, có số đo | `/* PERF(display_policy): 11 ms per full page; PSRAM reads dominate. */` |
| `REVIEW` | Cần người làm duyệt quyết định, hoặc chọn giữa các cách | `/* REVIEW(display): defaults not measured; tune with the bench firmware. */` |
| `EXTEND` | Chỗ nối khi mở rộng sau này | `/* EXTEND(input): add the IMU source to this switch. */` |
| `WARN` | Ràng buộc nguy hiểm, giữ lâu dài; sửa sai là hỏng | `/* WARN: DMA reads this buffer; keep it in internal RAM. */` |
| `NOTE` | Điều không hiển nhiên người sửa phải biết, nhưng không nguy hiểm | `/* NOTE: UC8253 swaps the two RAM areas after every DRF. */` |

Ví dụ trong bảng chỉ minh hoạ mẫu viết, không phải việc có thật trong code.

### Chọn tag khi phân vân

| Phân vân giữa | Chọn |
|---|---|
| `FIXME` hay `BUG` | Biết nguyên nhân: `FIXME`. Chưa biết: `BUG`, tìm ra nguyên nhân thì đổi sang `FIXME` |
| `TODO` hay `EXTEND` | Phải làm để chức năng hiện tại đủ: `TODO`. Chỉ cần khi thêm tính năng mới: `EXTEND` |
| `HACK` hay `FIXME` | Code chạy đúng nhưng cách làm xấu: `HACK`. Code chạy sai: `FIXME` |
| `WARN` hay `NOTE` | Sửa sai thì hỏng (dữ liệu, phần cứng, giao thức): `WARN`. Chỉ để hiểu nhanh hơn: `NOTE` |
| `REVIEW` hay `TODO` | Chưa biết nên làm gì, cần người làm quyết: `REVIEW`. Đã biết làm gì: `TODO` |

### Cách viết

- Mẫu: `TAG(scope): <việc>.` hoặc `TAG: <việc>.` `scope` nên có, là `id` của module trong glossary (`display`,
  `frame_slots`, `drv_uc8253`...), để lọc trong Better Todo Tree. `terms.py check` chỉ bắt buộc dấu `:` sau tag.
- Nội dung kiểm được: có số, điều kiện, hoặc tên hàm. "fix later" không đủ.
- Làm xong việc thì xoá tag trong cùng commit. `WARN` và `NOTE` giữ lâu dài.
- Cần tag mới: thêm vào bảng trên, vào `TAGS` trong `tools/terms.py`, vào `better-todo-tree.general.tags` và
  `better-todo-tree.highlights.customHighlight` trong `.vscode/settings.json`, trong cùng commit. Tag không có trong
  `general.tags` sẽ không hiện ở sidebar.

### Theo dõi tag bằng Better Todo Tree

Cài extension `FanaticPythoner.better-todo-tree` (bản fork còn bảo trì của Todo Tree; bản gốc trên Open VSX dừng ở 2022).
Cấu hình nằm trong `.vscode/settings.json`, key `better-todo-tree.*`.

- Sidebar liệt kê tag theo nhóm 9 tag ở bảng trên, kèm số lượng.
- Màu là 8 màu accent của Solarized (dùng chung cho Solarized Light và Dark), `NOTE` dùng base0 `#839496`. Tag in
  đậm, nền cùng màu độ mờ 15 %. Nhóm màu: đỏ/hồng/cam/vàng là lỗi và rủi ro (`FIXME`, `BUG`, `WARN`, `HACK`); xanh
  dương/tím/xanh lá là việc (`TODO`, `REVIEW`, `EXTEND`); xanh ngọc/xám là thông tin (`PERF`, `NOTE`).
- Chỉ quét `main/`, `components/`, `templates/`, `tools/*.py`, `test/`. Docs không bị quét, vì docs có ví dụ tag.
- Tag nằm trong comment Doxygen nhiều dòng (` * TODO(...)`) cũng được nhận.
