# Thiết kế: docs/design/display/bench.md
"""Root forwarder for display bench button CLI tool."""
import os
import sys

TARGET = os.path.join(os.path.dirname(os.path.abspath(__file__)), "display_bench", "button_cli.py")

if __name__ == "__main__":
    with open(TARGET, "rb") as f:
        code = compile(f.read(), TARGET, "exec")
        exec(code, globals())
