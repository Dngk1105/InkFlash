# Thiết kế: docs/design/code-structure.md
"""Tạo khối mới từ template.

    python tools/new.py <loại> <tên> --design docs/design/<file>.md

Loại: core (khối C thuần), shell (khối vỏ có task), driver (driver chip).
Ví dụ: python tools/new.py core gesture --design docs/design/input/input.md
"""
import argparse
import os
import re
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# loại -> (thư mục template, token tên trong template, thư mục đích, tiền tố tên component)
KINDS = {
    "core": ("templates/components/ink_example", "ink_example", "components/core", "ink_"),
    "shell": ("templates/components/ink_example_shell", "ink_example_shell", "components", "ink_"),
    "driver": ("templates/components/ink_drv_example", "ink_drv_example", "components/drivers", "ink_drv_"),
}

TEXT_EXT = (".c", ".h", ".txt", ".cmake")
DESIGN_RE = re.compile(r"Thiết kế: docs/[\w/.-]+\.md")
CMAKE_INCLUDE_RE = re.compile(r"include\(\$\{CMAKE_CURRENT_LIST_DIR\}/[./]*cmake/ink\.cmake\)")


def fail(msg):
    print(f"Lỗi: {msg}", file=sys.stderr)
    sys.exit(1)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Tạo khối mới từ template")
    parser.add_argument("kind", choices=KINDS.keys())
    parser.add_argument("name", help="tên khối, chữ thường và gạch dưới, không có tiền tố ink_")
    parser.add_argument("--design", required=True, help="file thiết kế khối này làm theo, ví dụ docs/design/input/input.md")
    args = parser.parse_args()

    if not re.fullmatch(r"[a-z][a-z0-9_]*", args.name) or args.name.startswith("ink_"):
        fail("tên chỉ gồm chữ thường, số, gạch dưới, bắt đầu bằng chữ, không có tiền tố ink_")
    if not os.path.isfile(os.path.join(ROOT, args.design)):
        fail(f"không có file thiết kế {args.design}; viết docs trước rồi mới tạo code")

    src_rel, token, dest_parent, prefix = KINDS[args.kind]
    new_name = prefix + args.name
    src = os.path.join(ROOT, src_rel)
    dest = os.path.join(ROOT, dest_parent, new_name)
    if os.path.exists(dest):
        fail(f"đã có {os.path.relpath(dest, ROOT)}")

    shutil.copytree(src, dest)
    cmake_rel = os.path.relpath(os.path.join(ROOT, "cmake", "ink.cmake"), dest).replace(os.sep, "/")
    for dirpath, _, files in os.walk(dest):
        for fname in files:
            path = os.path.join(dirpath, fname)
            if fname.endswith(TEXT_EXT):
                with open(path, encoding="utf-8") as f:
                    text = f.read()
                text = text.replace(token, new_name).replace(token.upper(), new_name.upper())
                text = DESIGN_RE.sub(f"Thiết kế: {args.design}", text, count=1)
                text = CMAKE_INCLUDE_RE.sub(f"include(${{CMAKE_CURRENT_LIST_DIR}}/{cmake_rel})", text)
                with open(path, "w", encoding="utf-8", newline="\n") as f:
                    f.write(text)
            if token in fname:
                os.rename(path, os.path.join(dirpath, fname.replace(token, new_name)))

    print(f"Đã tạo {os.path.relpath(dest, ROOT)} từ template {args.kind}.")
    print("Việc tiếp theo:")
    print("  1. Thay code mẫu bằng code thật, giữ nguyên khung (tên, file, dòng Thiết kế).")
    print("  2. Khai báo REQUIRES trong CMakeLists.txt theo bảng 'Được gọi' trong docs/architecture.md.")
    if args.kind == "driver":
        print("  3. Đổi ink_hal_example (interface mẫu) thành interface HAL thật mà driver làm theo.")
    print("  Để clangd thấy khối mới: chạy idf.py reconfigure (terminal ESP-IDF), rồi khởi động lại clangd.")
    print("  Cuối cùng: chạy python tools/check.py.")


if __name__ == "__main__":
    main()
