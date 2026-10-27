# InkFlash

Máy học flashcard bỏ túi: ESP32-S3, màn e-ink, 2 nút, chạy pin. ESP-IDF v6.1 + FreeRTOS.
Bản cũ: tag `skeleton-v2`, `demo-v0`.

## Đọc gì

Không đọc hết `docs/`. Bắt đầu từ `docs/architecture.md`: toàn cảnh, rồi bảng "Gặp vấn đề thì đọc file nào".

## Luật

- Repo được AI hỗ trợ viết khi dev yêu cầu; dev làm duyệt và tối ưu. Cần đảm bảo kết nối được mcp đầu phiên trước khi trả lời
- Code tuân thủ yêu cầu trong docs. Thấy code lệch docs hoặc lệch template: cảnh báo người làm và viết lại cho khớp.
- Kiến trúc chốt cùng người làm từng phần. Chưa chốt thì bàn trong chat, không ghi vào docs.
- Thông số phần cứng chưa có trong `docs/hardware/` thì hỏi, không đoán.
- Viết code ESP-IDF: chép từ `templates/`, không viết API theo trí nhớ. Tra thêm: docs offline v6.1 → MCP
  `espressif-docs` (bản mới nhất, phải đối chiếu lại v6.1) → web.
- Tạo khối mới bằng `tools/new.py`; `tools/check.py` phải qua trước khi báo xong.
- Build và kiểm tra toàn bộ: `python tools/build_all.py` trong terminal ESP-IDF.
- Build lẻ: `idf.py build (firmware)`, `idf.py -C templates build`.
- Terminal ESP-IDF trên máy này: trong PowerShell chạy `. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`
  (profile của EIM). Không dùng `C:\esp\v6.1\esp-idf\export.ps1`: nó tìm venv Python ở chỗ của cách cài cũ, nên lỗi.
- MCP `esp-idf`: `build_project` từng treo 30 phút ở lần build đầu: build lần đầu thì chạy`idf.py` trực tiếp, chạy nền.
- Viết hoặc sửa docs: đọc `docs/writing-docs.md` trước.
- Viết hoặc sửa comment trong code: đọc `docs/writing-comments.md` trước.
- Làm visualize hoặc file mô phỏng (`*_sim.html`): đọc `docs/writing-visualizations.md` trước.

<!-- agy-dieu-phoi -->
## Điều phối (giảm hạn mức Claude)
Cấu hình: `.claude/agy.json`. Trạng thái agy in ở đầu phiên (hook SessionStart).
| Tình huống                                                       | Làm gì                                  |
| -------------------------------------------------------------------- | ------------------------------------------- |
| Repo có MCP DocGraph                                              | `trace` / `pack` của DocGraph trước    |
| Cần đọc ≥ `scan_min_files` file, tóm tắt module, tìm luồng | skill `agy-scan`; lỗi → agent `scout`   |
| Code theo mẫu có sẵn, có test, > ~150 dòng                    | skill `agy-build` (theo `build_mode`)     |
| Sửa nhỏ < ~50 dòng, nghiệp vụ tinh tế, bảo mật             | Claude tự làm                           |
| Trước khi kết luận / sửa code                                 | Mở lại 1–3 `file:dòng` để xác minh |
- Đặc tả: `.claude/specs/<task>.md`. Log: `.claude/agy-runs/` (không commit).
- KHÔNG merge nhánh `agy/*` khi người dùng chưa đồng ý.
