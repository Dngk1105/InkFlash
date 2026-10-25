# Thiết kế: docs/design/code-structure.md
"""Kiểm tra code có khớp quy ước và docs không. Chạy trước mỗi commit:

    python tools/check.py

Lệch ở đâu thì báo lỗi kèm template đúng; code phải viết lại cho khớp (docs/design/code-structure.md).
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCAN_DIRS = ["main", "components", "templates", "cmake", "tools", "test/host"]
SKIP_DIRS = {"build", "managed_components", ".git", "__pycache__"}
CODE_EXT = (".c", ".h")
DESIGN_EXT = (".c", ".h", ".cmake", ".py")
DESIGN_RE = re.compile(r"Thiết kế: (docs/[\w/.-]+\.md)")

# API cấm (docs/design/esp-idf-api.md): (regex, gợi ý)
BANNED = [
    (r'#\s*include\s*[<"]driver/i2c\.h[>"]', "driver/i2c_master.h"),
    (r'#\s*include\s*[<"]driver/adc\.h[>"]', "esp_adc/adc_oneshot.h"),
    (r'#\s*include\s*[<"]esp_adc_cal\.h[>"]', "esp_adc/adc_cali_scheme.h"),
    (r'#\s*include\s*[<"]driver/i2s\.h[>"]', "driver/i2s_std.h"),
    (r'#\s*include\s*[<"]driver/timer\.h[>"]', "driver/gptimer.h"),
    (r'#\s*include\s*[<"]driver/(rmt|pcnt|mcpwm|sigmadelta)\.h[>"]', "driver mới tương ứng trong esp_driver_*"),
    (r"\besp_sleep_enable_ext1_wakeup\s*\(", "esp_sleep_enable_ext1_wakeup_io()"),
    (r"\besp_sleep_get_wakeup_cause\s*\(", "esp_sleep_get_wakeup_causes()"),
    (r'#\s*include\s*[<"]Arduino\.h[>"]', "API ESP-IDF"),
    (r"\b(digitalRead|digitalWrite|pinMode|analogRead)\s*\(", "driver/gpio.h hoặc esp_adc"),
    (r"\bmillis\s*\(\s*\)", "esp_timer_get_time()"),
    (r"(?<![\w.])delay\s*\(", "vTaskDelay(pdMS_TO_TICKS(ms))"),
    (r"\bSerial\.", "ESP_LOGx"),
]

C_STD_HEADERS = {
    "assert.h", "ctype.h", "errno.h", "float.h", "inttypes.h", "limits.h", "math.h", "stdalign.h", "stdarg.h",
    "stdatomic.h", "stdbool.h", "stddef.h", "stdint.h", "stdio.h", "stdlib.h", "string.h", "time.h",
}

TEMPLATE_HINT = {
    "core": "templates/components/ink_example",
    "shell": "templates/components/ink_example_shell",
    "driver": "templates/components/ink_drv_example",
}

errors = []


def err(path, line, msg):
    rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
    errors.append(f"{rel}:{line}: {msg}" if line else f"{rel}: {msg}")


def walk_files():
    for top in SCAN_DIRS:
        base = os.path.join(ROOT, top)
        if not os.path.isdir(base):
            continue
        for dirpath, dirnames, files in os.walk(base):
            # tools/: chỉ script của firmware nằm ngay trong tools/, không quét công cụ riêng ở thư mục con
            dirnames[:] = [] if top == "tools" else [d for d in dirnames if d not in SKIP_DIRS]
            for fname in files:
                yield os.path.join(dirpath, fname)


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def check_design_line(path, text):
    if not (path.endswith(DESIGN_EXT) or os.path.basename(path) == "CMakeLists.txt"):
        return
    head = "\n".join(text.splitlines()[:3])
    m = DESIGN_RE.search(head)
    if not m:
        err(path, 1, "thiếu dòng 'Thiết kế: docs/design/<file>.md' ở đầu file")
    elif not os.path.isfile(os.path.join(ROOT, m.group(1))):
        err(path, 1, f"dòng Thiết kế trỏ tới {m.group(1)} không tồn tại")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def check_banned(path, text):
    code = strip_comments(text)
    for lineno, line in enumerate(code.splitlines(), 1):
        for pattern, hint in BANNED:
            if re.search(pattern, line):
                err(path, lineno, f"API cấm, dùng {hint} (docs/design/esp-idf-api.md)")


def check_board_if(path, text):
    rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
    if "/ink_bsp/" in rel:
        return
    for lineno, line in enumerate(text.splitlines(), 1):
        if re.match(r"\s*#\s*(if|ifdef|ifndef|elif)\b.*BOARD", line):
            err(path, lineno, "#if BOARD chỉ được viết trong components/ink_bsp/ (docs/design/hardware-variants.md)")


PRIV_RE = re.compile(r"(\.|->)\s*priv\b")


def check_priv(path, text, core_dirs):
    """Trường `priv` của khối lõi chỉ được truy cập trong thư mục khối lõi. Chưa phân biệt khối lõi này đụng `priv`
    của khối lõi khác."""
    if any(os.path.normpath(path).startswith(d + os.sep) for d in core_dirs):
        return
    for lineno, line in enumerate(strip_comments(text).splitlines(), 1):
        if PRIV_RE.search(line):
            err(path, lineno, "chỉ khối lõi được đụng trường priv của nó; dùng hàm của khối "
                "(docs/design/code-structure.md)")


def component_dirs():
    for base in ("components", "templates/components"):
        top = os.path.join(ROOT, base)
        if not os.path.isdir(top):
            continue
        for dirpath, dirnames, files in os.walk(top):
            dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
            if "CMakeLists.txt" in files:
                yield dirpath
                dirnames[:] = []


def check_component(comp):
    name = os.path.basename(comp)
    cmake = read(os.path.join(comp, "CMakeLists.txt"))
    is_core = "ink_core_component(" in cmake
    kind = "core" if is_core else ("driver" if name.startswith("ink_drv_") else "shell")
    hint = TEMPLATE_HINT[kind]

    if not name.startswith("ink_"):
        err(comp, 0, f"tên component phải có tiền tố ink_ (mẫu: {hint})")
    header = os.path.join(comp, "include", name + ".h")
    if not os.path.isfile(header):
        err(comp, 0, f"thiếu header công khai include/{name}.h (mẫu: {hint})")
    if is_core and not any(f.endswith(".c") for f in os.listdir(os.path.join(comp, "test"))
                           if os.path.isdir(os.path.join(comp, "test"))):
        err(comp, 0, f"khối C thuần phải có host test trong test/*.c (mẫu: {TEMPLATE_HINT['core']}/test)")

    inc_dir = os.path.join(comp, "include")
    if os.path.isdir(inc_dir):
        for fname in os.listdir(inc_dir):
            if fname.endswith(".h"):
                check_public_names(os.path.join(inc_dir, fname), name, hint)

    if is_core:
        for dirpath, _, files in os.walk(comp):
            for fname in files:
                if fname.endswith(CODE_EXT):
                    check_core_includes(os.path.join(dirpath, fname), hint)


def check_public_names(path, comp, hint):
    if not os.path.basename(path).startswith(comp):
        err(path, 0, f"tên header phải bắt đầu bằng {comp} (mẫu: {hint})")
    code = strip_comments(read(path))
    upper = comp.upper()
    for lineno, line in enumerate(code.splitlines(), 1):
        if line[:1].isspace():
            continue
        m = re.match(r"[A-Za-z_][\w\s\*]*?\b(\w+)\s*\(", line)
        if m and not line.startswith(("#", "typedef")) and not m.group(1).startswith(comp + "_"):
            err(path, lineno, f"hàm public '{m.group(1)}' phải bắt đầu bằng {comp}_ (mẫu: {hint})")
        t = re.match(r"}\s*(\w+)\s*;", line)
        if t and not (t.group(1).startswith(comp + "_") and t.group(1).endswith("_t")):
            err(path, lineno, f"kiểu '{t.group(1)}' phải có dạng {comp}_<tên>_t (mẫu: {hint})")
        d = re.match(r"#\s*define\s+(\w+)", line)
        if d and not d.group(1).startswith(upper + "_"):
            err(path, lineno, f"macro '{d.group(1)}' phải bắt đầu bằng {upper}_ (mẫu: {hint})")
    for lineno, line in enumerate(code.splitlines(), 1):
        for value in re.findall(r"^\s+([A-Z][A-Z0-9_]+)\s*(?:=[^,]*)?,", line):
            if not value.startswith(upper + "_"):
                err(path, lineno, f"giá trị enum '{value}' phải bắt đầu bằng {upper}_ (mẫu: {hint})")


def check_core_includes(path, hint):
    for lineno, line in enumerate(read(path).splitlines(), 1):
        m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
        if not m:
            continue
        inc = m.group(1)
        if inc in C_STD_HEADERS or inc.startswith("ink_") or inc == "unity.h":
            continue
        err(path, lineno, f"khối C thuần không được include '{inc}' (chỉ thư viện C chuẩn và khối ink_*; mẫu: {hint})")


def find_clang_format():
    exe = shutil.which("clang-format")
    if exe:
        return exe
    base = r"C:\Espressif\tools\esp-clang"
    if os.path.isdir(base):
        for ver in sorted(os.listdir(base), reverse=True):
            cand = os.path.join(base, ver, "esp-clang", "bin", "clang-format.exe")
            if os.path.isfile(cand):
                return cand
    return None


def check_format(files):
    exe = find_clang_format()
    if not exe:
        errors.append("không tìm thấy clang-format (có sẵn trong C:\\Espressif\\tools\\esp-clang)")
        return
    for path in files:
        r = subprocess.run([exe, "--dry-run", "-Werror", f"--style=file:{os.path.join(ROOT, '.clang-format')}", path],
                           capture_output=True, text=True)
        if r.returncode != 0:
            err(path, 0, f"sai định dạng; sửa bằng: clang-format -i {os.path.relpath(path, ROOT)}")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    core_dirs = [os.path.normpath(c) for c in component_dirs()
                 if "ink_core_component(" in read(os.path.join(c, "CMakeLists.txt"))]
    code_files = []
    for path in walk_files():
        text = read(path) if path.endswith((".c", ".h", ".cmake", ".py", ".txt")) else ""
        check_design_line(path, text)
        if path.endswith(CODE_EXT):
            code_files.append(path)
            check_banned(path, text)
            check_board_if(path, text)
            check_priv(path, text, core_dirs)
    for comp in component_dirs():
        check_component(comp)
    check_format(code_files)

    if errors:
        print("\n".join(errors))
        print(f"\n{len(errors)} lỗi. Viết lại code cho khớp template và docs.")
        sys.exit(1)
    print(f"OK: {len(code_files)} file C, không có lỗi.")


if __name__ == "__main__":
    main()
