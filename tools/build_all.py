# Thiết kế: docs/design/code-structure.md
"""Build và kiểm tra toàn bộ repo. Chạy trong terminal ESP-IDF (idf.py có trong PATH):

    python tools/build_all.py

Các bước: tools/check.py → host test (gcc trên PC) → build templates/ → build firmware.
Dừng ở bước đầu tiên lỗi.
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_IDF_PATH = r"C:\esp\v6.1\esp-idf"
NINJA_FALLBACK = r"C:\Espressif\tools\ninja\1.12.1"


def run(title, cmd, env=None):
    print(f"\n== {title}: {' '.join(cmd)}", flush=True)
    if subprocess.run(cmd, cwd=ROOT, env=env).returncode != 0:
        print(f"\nLỗi ở bước: {title}")
        sys.exit(1)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    env = dict(os.environ)
    env.setdefault("IDF_PATH", DEFAULT_IDF_PATH)
    if not shutil.which("ninja", path=env.get("PATH")) and os.path.isdir(NINJA_FALLBACK):
        env["PATH"] = NINJA_FALLBACK + os.pathsep + env.get("PATH", "")

    run("Kiểm tra quy ước", [sys.executable, "tools/check.py"])
    run("Cấu hình host test", ["cmake", "-S", "test/host", "-B", "build-host", "-G", "Ninja",
                               "-DCMAKE_C_COMPILER=gcc"], env)
    run("Build host test", ["cmake", "--build", "build-host"], env)
    run("Chạy host test", ["ctest", "--test-dir", "build-host", "--output-on-failure"], env)

    idf = shutil.which("idf.py") or shutil.which("idf.py.exe")
    if not idf:
        print("\nKhông thấy idf.py: mở terminal ESP-IDF v6.1 rồi chạy lại để build templates/ và firmware.")
        sys.exit(1)
    run("Build templates/", [idf, "-C", "templates", "build"], env)
    run("Build firmware", [idf, "build"], env)
    print("\nTất cả đều qua.")


if __name__ == "__main__":
    main()
