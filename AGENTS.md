# InkFlash

Máy học flashcard bỏ túi: ESP32-S3, màn e-ink, 2 nút, chạy pin. ESP-IDF v6.1 + FreeRTOS.
Bản cũ: tag `skeleton-v2`, `demo-v0`.

File này là luật chung cho mọi agent (Claude, Antigravity, Codex, Gemini). Luật riêng của Claude nằm trong `CLAUDE.md`.

## Đọc gì

Không đọc hết `docs/`. Bắt đầu từ `docs/architecture.md`: toàn cảnh, rồi bảng "Gặp vấn đề thì đọc file nào".

## Luật

- Không commit nhánh `main` nếu chưa được người làm cho phép.
- Repo được AI hỗ trợ viết khi người làm yêu cầu; người làm duyệt và tối ưu. Đầu phiên kiểm tra MCP kết nối được trước khi trả lời.
- Code tuân thủ docs. Thấy code lệch docs hoặc lệch template: cảnh báo người làm và viết lại cho khớp.
- Kiến trúc chốt cùng người làm từng phần. Chưa chốt thì bàn trong chat, không ghi vào docs.
- Thông số phần cứng chưa có trong `docs/hardware/` thì hỏi, không đoán.
- Viết code ESP-IDF: chép từ `templates/`, không viết API theo trí nhớ. Tra thêm: docs offline v6.1 → MCP
  `espressif-docs` (bản mới nhất, phải đối chiếu lại v6.1) → web.
- Tạo khối mới bằng `tools/new.py`; `tools/check.py` phải qua trước khi báo xong.
- Build và kiểm tra toàn bộ: `python tools/build_all.py` trong terminal ESP-IDF.
- Build lẻ: `idf.py build` (firmware), `idf.py -C templates build`.
- Terminal ESP-IDF trên máy này: trong PowerShell chạy `. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`
  (profile của EIM). Không dùng `C:\esp\v6.1\esp-idf\export.ps1`: nó tìm venv Python ở chỗ của cách cài cũ, nên lỗi.
- MCP `esp-idf`: `build_project` từng treo 30 phút ở lần build đầu. Build lần đầu thì chạy `idf.py` trực tiếp, chạy nền.
- Kiểm thử hiển thị e-ink bằng camera (iPhone qua Lightning/Webcam): gọi `python tools/vision_test/capture.py -o <subpath> --json`, xem ảnh trả về bằng tool đọc file/image để đối chiếu màn hình thực tế. Phân tích chỉ số định lượng (tương phản, độ nét, ghosting debt) bằng `python tools/vision_test/analyze.py -i <image.jpg> --json`. Xem hướng dẫn tại `tools/vision_test/README.md`.

## Viết comment, docs, chat

| Việc | Đọc trước |
|---|---|
| Comment trong code (tiếng Anh, Doxygen, tag `TODO`/`FIXME`/`BUG`/`HACK`/`PERF`/`REVIEW`/`EXTEND`/`WARN`/`NOTE`) | `docs/writing-comments.md` |
| Docs (tiếng Việt, thuật ngữ tiếng Anh, 5C) | `docs/writing-docs.md` |
| Thuật ngữ, đổi tên identifier | `docs/glossary.yaml` |
| Visualize, file mô phỏng (`*_sim.html`) | `docs/writing-visualizations.md` |
| Giọng văn tiếng Việt (chat, docs, 5C) | skill `inkflash-vn` |

- Chat với người làm: tiếng Việt, câu ngắn, thuật ngữ viết đúng `term` trong `docs/glossary.yaml`. Không dịch `frame`, `display task`, `debt`... sang tiếng Việt.
- Trước khi báo xong: `python tools/terms.py check` (thuật ngữ, comment, tag, Doxygen) và `python tools/check.py` phải qua. Cả hai cần PyYAML, có sẵn trong terminal ESP-IDF.
- Đổi tên identifier: sửa `docs/glossary.yaml`, rồi `python tools/terms.py rename` (xem diff), `--apply` (ghi). Không đổi tên bằng tay.
- Thấy việc còn thiếu, lỗi, chỗ tạm, chỗ chậm, chỗ cần người làm duyệt khi đang sửa code: gắn tag ngay tại dòng đó
  (bảng "Tag việc dở" trong `docs/writing-comments.md`), không cần hỏi trước.
