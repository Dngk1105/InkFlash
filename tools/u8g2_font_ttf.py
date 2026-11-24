# Design: docs/design/ui.md
"""Sinh font cho ink_ui từ một font pixel TTF (đường nét thẳng, lưới pixel đều), phóng nguyên N lần.

  python tools/u8g2_font_ttf.py Tiny5-Regular.ttf tiny5_16 --scale 2

Các bước:
  1. Vẽ mỗi glyph lên lưới pixel của font (1 pixel = em / grid_em đơn vị, mặc định grid_em = 8), lấy mẫu ở tâm pixel.
  2. Phóng mỗi pixel thành N × N rồi ghi file BDF tạm.
  3. Chạy bdfconv của U8g2 (third_party/u8g2/tools/font/bdfconv/bdfconv.exe) ra mảng C định dạng U8g2.
  4. Bọc mảng trong biến ink_ui_font_<name> ở components/core/ink_ui/fonts/ink_ui_font_<name>.c.

Cần fontTools (pip install fonttools). Sinh xong thì thêm file vào SRCS của components/core/ink_ui/CMakeLists.txt
và khai báo biến trong ink_ui.h. Giấy phép font ghi trong third_party/fonts/<font>/.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

from fontTools.pens.recordingPen import DecomposingRecordingPen
from fontTools.ttLib import TTFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BDFCONV = os.path.join(ROOT, "third_party", "u8g2", "tools", "font", "bdfconv", "bdfconv.exe")
OUT_DIR = os.path.join(ROOT, "components", "core", "ink_ui", "fonts")

# Latin-1, Vietnamese letters (including Ă, Đ, Ĩ, Ũ, Ơ, Ư and U+1EA0..U+1EF9), IPA, basic punctuation, dong sign, arrows.
# A narrow set keeps the line height set by tone marks stacked on one letter, not by rare Latin Extended glyphs.
RANGES = [(0x20, 0x7E), (0xA0, 0xFF), (0x102, 0x103), (0x110, 0x111), (0x128, 0x129), (0x168, 0x169), (0x1A0, 0x1A1), (0x1AF, 0x1B0),
          (0x250, 0x2AF), (0x2C8, 0x2C8), (0x2D0, 0x2D1), (0x1EA0, 0x1EF9), (0x2010, 0x2027), (0x20AB, 0x20AB), (0x2190, 0x2193)]


def outline(glyph_set, name):
    """Polygons of a glyph, in font units. Exits when the outline has curves."""
    pen = DecomposingRecordingPen(glyph_set)
    glyph_set[name].draw(pen)
    polygons, current = [], []
    for op, args in pen.value:
        if op == "moveTo":
            current = [args[0]]
        elif op == "lineTo":
            current.append(args[0])
        elif op == "qCurveTo" and args[-1] is None:
            current = list(args[:-1])  # TrueType contour with on-curve points only
        elif op == "qCurveTo" and len(set(args)) == 1:
            current.append(args[-1])  # degenerate curve (all points equal), e.g. Tiny5 with ROND = 0
        elif op in ("closePath", "endPath"):
            if current:
                polygons.append(current)
            current = []
        else:
            sys.exit(f"u8g2_font_ttf.py: glyph '{name}' có đường cong ({op}); script chỉ nhận font pixel nét thẳng")
    return polygons


def winding(polygons, x, y):
    """Nonzero winding number of point (x, y); horizontal ray to +x."""
    total = 0
    for poly in polygons:
        for (x0, y0), (x1, y1) in zip(poly, poly[1:] + poly[:1]):
            if (y0 <= y) != (y1 <= y):
                if x0 + (y - y0) * (x1 - x0) / (y1 - y0) > x:
                    total += 1 if y1 > y0 else -1
    return total


def raster(polygons, unit):
    """Rows (top to bottom) of 0/1 pixels and (x_offset, y_bottom) in grid pixels; None when the glyph is empty."""
    points = [p for poly in polygons for p in poly]
    if not points:
        return None
    x_lo = int(min(p[0] for p in points) // unit)
    x_hi = int(-(-max(p[0] for p in points) // unit))
    y_lo = int(min(p[1] for p in points) // unit)
    y_hi = int(-(-max(p[1] for p in points) // unit))
    rows = []
    for gy in range(y_hi - 1, y_lo - 1, -1):
        rows.append([1 if winding(polygons, (gx + 0.5) * unit, (gy + 0.5) * unit) else 0 for gx in range(x_lo, x_hi)])
    while rows and not any(rows[0]):
        rows.pop(0)
        y_hi -= 1
    while rows and not any(rows[-1]):
        rows.pop()
        y_lo += 1
    if not rows:
        return None
    cols = [i for i in range(len(rows[0])) if any(r[i] for r in rows)]
    first, last = cols[0], cols[-1]
    rows = [r[first : last + 1] for r in rows]
    return rows, x_lo + first, y_lo


def bdf_text(font, name, scale, grid_em):
    cmap = font.getBestCmap()
    glyph_set = font.getGlyphSet()
    unit = font["head"].unitsPerEm / grid_em
    entries = []
    for lo, hi in RANGES:
        for code in range(lo, hi + 1):
            if code in cmap:
                gname = cmap[code]
                advance = round(font["hmtx"][gname][0] / unit) * scale
                entries.append((code, advance, raster(outline(glyph_set, gname), unit)))
    boxes = [(r[1], r[2], len(r[0][0]), len(r[0])) for _, _, r in entries if r]
    x_min = min(b[0] for b in boxes) * scale
    y_min = min(b[1] for b in boxes) * scale
    x_max = max((b[0] + b[2]) * scale for b in boxes)
    y_max = max((b[1] + b[3]) * scale for b in boxes)
    ascent, descent = y_max, -y_min
    out = ["STARTFONT 2.1", f"FONT {name}", f"SIZE {grid_em * scale} 75 75", f"FONTBOUNDINGBOX {x_max - x_min} {y_max - y_min} {x_min} {y_min}",
           "STARTPROPERTIES 2", f"FONT_ASCENT {ascent}", f"FONT_DESCENT {descent}", "ENDPROPERTIES", f"CHARS {len(entries)}"]
    for code, advance, r in entries:
        out += [f"STARTCHAR U+{code:04X}", f"ENCODING {code}", f"SWIDTH {advance * 1000 // (grid_em * scale)} 0", f"DWIDTH {advance} 0"]
        if r is None:
            out += ["BBX 0 0 0 0", "BITMAP", "ENDCHAR"]
            continue
        rows, x0, y0 = r
        width, height = len(rows[0]) * scale, len(rows) * scale
        out += [f"BBX {width} {height} {x0 * scale} {y0 * scale}", "BITMAP"]
        for row in rows:
            bits = "".join(str(b) * scale for b in row).ljust((width + 7) // 8 * 8, "0")
            line = f"{int(bits, 2):0{len(bits) // 4}X}"
            out += [line] * scale
        out.append("ENDCHAR")
    out.append("ENDFONT")
    return "\n".join(out) + "\n", ascent + descent


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description="Sinh font ink_ui từ font pixel TTF")
    parser.add_argument("ttf", help="file TTF của font pixel")
    parser.add_argument("name", help="tên ngắn, chữ thường và gạch dưới; biến sinh ra là ink_ui_font_<name>")
    parser.add_argument("--scale", type=int, default=1, help="phóng nguyên N lần (mặc định 1)")
    parser.add_argument("--grid-em", type=int, default=8, help="số pixel của font trong một em (Tiny5: 8)")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z][a-z0-9_]*", args.name):
        sys.exit("u8g2_font_ttf.py: tên ngắn chỉ gồm chữ thường, số, gạch dưới")
    if not os.path.isfile(BDFCONV):
        sys.exit(f"u8g2_font_ttf.py: thiếu {BDFCONV}; chạy git submodule update --init")

    text, line_height = bdf_text(TTFont(args.ttf), args.name, args.scale, args.grid_em)
    with tempfile.TemporaryDirectory() as tmp:
        bdf, raw = os.path.join(tmp, args.name + ".bdf"), os.path.join(tmp, args.name + ".c")
        with open(bdf, "w", newline="\n") as f:
            f.write(text)
        ranges = ",".join(f"{lo}-{hi}" for lo, hi in RANGES)
        subprocess.run([BDFCONV, "-b", "0", "-f", "1", "-m", ranges, "-n", args.name, "-o", raw, bdf], check=True, stdout=subprocess.DEVNULL)
        with open(raw, encoding="latin-1") as f:
            source = f.read()
    head = re.search(r"\[(\d+)\][^=]*=\s*\n", source)
    size = int(head.group(1))
    body = [line.strip() for line in source[head.end() :].splitlines() if line.strip().startswith('"') or line.strip().endswith(";")]
    body = [line for line in body if line.startswith('"')]
    body[-1] = body[-1].rstrip(";")
    out = [
        "/* Design: docs/design/ui.md */",
        "",
        f"/* Generated by tools/u8g2_font_ttf.py from {os.path.basename(args.ttf)} (scale {args.scale}). Do not edit.",
        " * Font license: third_party/fonts/ (SIL Open Font License 1.1). */",
        '#include "ink_ui.h"',
        "",
        "/* clang-format off */",
        '#pragma GCC diagnostic ignored "-Woverlength-strings"',
        f"static const uint8_t data[{size}] =",
    ]
    out += ["    " + line for line in body[:-1]] + ["    " + body[-1] + ";", "/* clang-format on */", "", f"const ink_ui_font_t ink_ui_font_{args.name} = {{.data = data}};", ""]
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, f"ink_ui_font_{args.name}.c")
    with open(path, "w", encoding="latin-1", newline="\n") as f:
        f.write("\n".join(out))
    print(f"Đã sinh {os.path.relpath(path, ROOT)} ({size} byte, dòng cao {line_height} px)")


if __name__ == "__main__":
    main()
