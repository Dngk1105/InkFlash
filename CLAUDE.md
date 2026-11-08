# InkFlash

Máy học flashcard bỏ túi: ESP32-S3, màn e-ink, 2 nút, chạy pin. ESP-IDF v6.1 + FreeRTOS.
Bản cũ: tag `skeleton-v2`, `demo-v0`.

## Đọc gì

Không đọc hết `docs/`. Bắt đầu từ `docs/architecture.md`: toàn cảnh, rồi bảng "Gặp vấn đề thì đọc file nào".

## Luật

- Không commit nhánh main nếu chưa được sự cho phép
- Repo được AI hỗ trợ viết khi dev yêu cầu; dev làm duyệt và tối ưu. Cần đảm bảo kết nối được mcp đầu phiên trước khi trả lời.
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

<!-- CODELENS_MANAGED_START -->
## CodeLens Graph — Mandatory Search Protocol

This workspace has a live codebase knowledge graph via **CodeLens Graph** MCP, updated on every save.
Run `codelens_status` for current symbol and file counts.

### RULE 1 — Triage first to establish the baseline
Before starting a task, call `codelens_triage` to classify it.
Use the triage response to pick the most efficient tool path. You have full flexibility to choose other tools as necessary:
- **Tier 1 (typo/formatting)**: No tools needed.
- **Tier 2 (symbol lookup / search)**: Call `codelens_search` (for classes, functions, types) or `codelens_text_search` (for strings, comments, local variables).
- **Tier 3 (features / bugfixes)**: Start with `codelens_context`. Use `mode: "short"` to quickly see the file/symbol map (cheapest), or `mode: "deep"` only if you need full implementations.
- **Tier 4 (refactoring)**: Use `codelens_context` + `codelens_impact` to map dependencies and prevent breaking changes.

### RULE 2 — Use specific tools instead of scanning files
Avoid generic workspace scans (grep, ls, find) or reading whole files. Use these targeted tools:
| Task / Need | Recommended Tool | Why It Saves Tokens |
|---|---|---|
| Locate symbol definition | `codelens_search` | Returns exact file:line + signature |
| Search text, comments, or strings | `codelens_text_search` | Searches line-by-line using fuzzy text index |
| Inspect 1 class/function code | `codelens_node` (with `with_snippet: true`) | Avoids reading the whole file containing it |
| Understand feature context | `codelens_context` | Returns a minimal subgraph of only related files |
| Find callers/callees of a function | `codelens_relations` | Lists callers, callees, or both for a given symbol |
| See transitive dependencies | `codelens_impact` | Automatically runs BFS to map the blast radius |
| Check directory structure | `codelens_files` | Returns category-grouped file list |

### RULE 3 — Read only what CodeLens points to
When CodeLens tools return a `file:line` range, read only that specific range using the `view_file` tool (with StartLine and EndLine).
Do NOT read whole files, and never read files that are not listed in the graph response.

### RULE 4 — Check before creating
Before writing a new function, class, or file, run `codelens_search` to ensure you are not creating a duplicate. Duplication is the #1 source of code rot.

### RULE 5 — Keep the graph updated
The knowledge graph is updated automatically on file save. You can run `codelens_status` to verify the index is healthy and up to date.

### RULE 6 — Query dependencies and configs strategically
Only search for package dependencies, type definitions, or configuration files (like package.json, tsconfig.json) when asked or if context is missing.
Use `codelens_search` or `codelens_files` with `scope: "deps"`, or use `codelens_dependencies` directly.

### Available tools (MCP server: codelens)
`codelens_triage` · `codelens_search` · `codelens_context` · `codelens_dependencies`
`codelens_relations` · `codelens_impact` · `codelens_text_search`
`codelens_node` · `codelens_files` · `codelens_status`
<!-- CODELENS_MANAGED_END -->
