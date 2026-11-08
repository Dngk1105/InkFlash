# InkFlash (Claude)

@AGENTS.md

Luật chung ở `AGENTS.md`. Phần dưới chỉ dành cho Claude.

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