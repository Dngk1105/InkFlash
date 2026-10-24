# Thiết kế: docs/design/code-structure.md
"""Xoá cờ -m* và -f* khỏi compile_commands.json để clangd không báo "Unknown argument".

CMake sinh file này cho GCC của Xtensa; clang không biết các cờ như -mdisable-hardware-atomics.
.clangd ở gốc repo chỉ áp dụng cho file trong repo, còn file ESP-IDF (C:/esp/...) thì cần sửa thẳng file này
(cách Espressif khuyên: https://docs.espressif.com/projects/espressif-ide/en/latest/additionalfeatures/clangd_cdt_support.html).

Dùng: python tools/fix_compile_commands.py build/compile_commands.json
Hàm ink_fix_clangd() trong cmake/ink.cmake tự chạy nó sau mỗi lần build.
"""
import json
import shlex
import sys


def keep(arg):
    return not (arg.startswith("-m") or arg.startswith("-f"))


def main():
    path = sys.argv[1]
    with open(path, encoding="utf-8") as f:
        entries = json.load(f)

    for entry in entries:
        if "arguments" in entry:
            entry["arguments"] = [a for a in entry["arguments"] if keep(a)]
        if "command" in entry:
            entry["command"] = " ".join(a for a in shlex.split(entry["command"], posix=False) if keep(a))

    with open(path, "w", encoding="utf-8") as f:
        json.dump(entries, f, indent=2)
    print(f"fix_compile_commands: cleaned {len(entries)} entries in {path}")


if __name__ == "__main__":
    main()
