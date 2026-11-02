# Thiết kế: docs/design/display/bench.md
"""Phục vụ trang công cụ test màn và ghi kết quả chấm điểm. Chỉ dùng thư viện chuẩn của Python.

    python tools/display_bench/bench_server.py [--port 8253]

Mở http://localhost:8253 trên Chrome hoặc Edge (Web Serial chỉ chạy trên localhost).
"""
import argparse
import datetime
import http.server
import json
import os
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
WEB_DIR = os.path.join(ROOT, "web")
RESULTS_DIR = os.path.join(ROOT, "results")
IMAGES_DIR = os.path.join(ROOT, "images")
IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp")
MAX_RESULT_BYTES = 64 * 1024


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    # ---- Trả JSON ----

    def send_json(self, status, value):
        body = json.dumps(value, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    # ---- GET ----

    def do_GET(self):
        if self.path == "/api/results":
            self.send_json(200, read_results())
        elif self.path == "/api/images":
            self.send_json(200, list_images())
        elif self.path.startswith("/images/"):
            self.send_image(self.path[len("/images/"):])
        else:
            super().do_GET()

    def send_image(self, name):
        path = os.path.join(IMAGES_DIR, os.path.basename(name))
        if not name.lower().endswith(IMAGE_EXTENSIONS) or not os.path.isfile(path):
            self.send_error(404)
            return
        with open(path, "rb") as file:
            body = file.read()
        self.send_response(200)
        self.send_header("Content-Type", self.guess_type(path))
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    # ---- POST ----

    def do_POST(self):
        if self.path != "/api/results":
            self.send_error(404)
            return
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0 or length > MAX_RESULT_BYTES:
            self.send_json(400, {"error": "độ dài không hợp lệ"})
            return
        try:
            result = json.loads(self.rfile.read(length))
        except json.JSONDecodeError:
            self.send_json(400, {"error": "JSON sai"})
            return
        self.send_json(200, {"file": append_result(result)})

    def log_message(self, format, *args):
        if "/api/" in self.path:
            super().log_message(format, *args)


# ---- Kết quả chấm điểm: mỗi dòng một JSON, mỗi ngày một file ----


def append_result(result):
    os.makedirs(RESULTS_DIR, exist_ok=True)
    now = datetime.datetime.now()
    result["saved_at"] = now.isoformat(timespec="seconds")
    name = now.strftime("%Y-%m-%d") + ".jsonl"
    with open(os.path.join(RESULTS_DIR, name), "a", encoding="utf-8", newline="\n") as file:
        file.write(json.dumps(result, ensure_ascii=False) + "\n")
    return name


def read_results():
    results = []
    if not os.path.isdir(RESULTS_DIR):
        return results
    for name in sorted(os.listdir(RESULTS_DIR)):
        if not name.endswith(".jsonl"):
            continue
        with open(os.path.join(RESULTS_DIR, name), encoding="utf-8") as file:
            for line in file:
                line = line.strip()
                if line:
                    try:
                        results.append(json.loads(line))
                    except json.JSONDecodeError:
                        pass
    return results


def list_images():
    if not os.path.isdir(IMAGES_DIR):
        return []
    return sorted(name for name in os.listdir(IMAGES_DIR) if name.lower().endswith(IMAGE_EXTENSIONS))


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Công cụ test màn InkFlash")
    parser.add_argument("--port", type=int, default=8253)
    port = parser.parse_args().port
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"Mở http://localhost:{port} trên Chrome hoặc Edge. Ctrl+C để dừng.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
