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

## Tag việc dở

Docs chỉ mô tả kiến trúc hiện tại. Việc còn thiếu, lỗi đã biết, chỗ cần duyệt, điểm mở rộng ghi bằng comment có tag, ngay tại dòng code liên quan.

| Tag | Nghĩa | Ví dụ |
|---|---|---|
| `TODO` | Thiếu chức năng | `/* TODO(display): partial draw for windows narrower than 8 px. */` |
| `FIXME` | Lỗi đã biết, chưa sửa | `/* FIXME(drv_uc8253): needs_reset not cleared after a timeout in power_off. */` |
| `REVIEW` | Cần người làm duyệt hoặc tối ưu | `/* REVIEW(frame_slots): lock held across 2 calls; measure wait time. */` |
| `EXTEND` | Chỗ nối khi mở rộng | `/* EXTEND(input): add IMU source to this switch. */` |
| `WARN` | Ràng buộc nguy hiểm, giữ lâu dài | `/* WARN: DMA reads this buffer; keep it in internal RAM. */` |

Ví dụ trong bảng chỉ minh hoạ mẫu viết, không phải việc có thật trong code.

- Mẫu bắt buộc: `TAG(scope): <việc>.` `scope` là `id` của module trong glossary (`display`, `frame_slots`, `drv_uc8253`...). `WARN` được bỏ `scope`.
- Nội dung phải kiểm được: có số, điều kiện, hoặc tên hàm. Viết "fix later" là sai.
- Tag khác (`XXX`, `HACK`, `BUG`, `NOTE`) bị `terms.py check` báo lỗi.
- Làm xong việc thì xoá tag trong cùng commit.
