# Design: docs/design/figma-setup.md
"""Đổi một font U8g2 (mảng trong third_party/u8g2/csrc/u8g2_fonts.c) thành TTF để dùng trong Figma.

Firmware vẽ chữ bằng chính mảng U8g2 này. Script giải mã từng glyph của mảng rồi vẽ mỗi pixel thành ô vuông
trong TTF, nên Figma vẽ đúng bitmap mà panel vẽ. Cỡ chữ trong Figma = line height của font (ascent + descent
theo u8g2_SetFontRefHeightAll), khi đó 1 pixel font = 1 pixel Figma.

Font sinh ra không có kerning và có ascent/descent đúng như firmware, nên Figma đặt baseline và độ rộng chữ đúng như
panel. Một TTF gốc (ví dụ Tiny5 của Google Fonts) thì khác: Figma áp kerning và đặt baseline theo metric của TTF.

Cách dùng (cần fontTools):
    python tools/figma2ink/make_u8g2_ttf.py u8g2_font_6x10_tf --family "Fixed 6x10" -o fixed-6x10.ttf --install
    python tools/figma2ink/make_u8g2_ttf.py components/core/ink_ui/fonts/ink_ui_font_tiny5_8.c --family "Tiny5 Ink" \
        -o tiny5-ink.ttf --install

--install chép file vào thư mục font của người dùng và đăng ký với Windows, như make_unifont_ttf.py.
"""
import argparse
import os
import re
import sys

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from u8g2_font import find_font  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_unifont_ttf import install  # noqa: E402

UNIT = 100  # font units per pixel
HEADER_SIZE = 23


def c_bytes(lines):
    """Join the C string literal lines of a U8g2 array into bytes (octal and simple escapes)."""
    out = bytearray()
    for line in lines:
        for body in re.findall(r'"((?:[^"\\]|\\.)*)"', line):
            i = 0
            while i < len(body):
                ch = body[i]
                if ch != "\\":
                    out.append(ord(ch))
                    i += 1
                    continue
                m = re.match(r"[0-7]{1,3}", body[i + 1:])
                if m:
                    out.append(int(m.group(0), 8))
                    i += 1 + len(m.group(0))
                else:
                    esc = body[i + 1]
                    out.append({"n": 10, "t": 9, "r": 13}.get(esc, ord(esc)))
                    i += 2
    return bytes(out)


class Bits:
    """LSB-first bit reader, as u8g2_font_decode_get_unsigned_bits."""

    def __init__(self, data, pos):
        self.data, self.pos, self.bit = data, pos, 0

    def u(self, count):
        val = 0
        for i in range(count):
            val |= ((self.data[self.pos] >> self.bit) & 1) << i
            self.bit += 1
            if self.bit == 8:
                self.bit, self.pos = 0, self.pos + 1
        return val

    def s(self, count):
        return self.u(count) - (1 << (count - 1)) if count else 0


def s8(b):
    return b - 256 if b > 127 else b


def parse_header(data):
    keys = ("glyph_cnt", "bbx_mode", "b0", "b1", "bw", "bh", "bx", "by", "bdx", "max_w", "max_h")
    h = dict(zip(keys, data[:11]))
    h["x_offset"], h["y_offset"] = s8(data[11]), s8(data[12])
    h["ascent_A"], h["descent_g"] = s8(data[13]), s8(data[14])
    h["start_unicode"] = (data[21] << 8) | data[22]
    return h


def decode_glyph(h, bits):
    """Return (bbx x, bbx y of the bottom row, rows top to bottom, advance) of one glyph (u8g2_font_decode_glyph)."""
    w, hh = bits.u(h["bw"]), bits.u(h["bh"])
    x, y, dx = bits.s(h["bx"]), bits.s(h["by"]), bits.s(h["bdx"])
    rows = [[0] * w for _ in range(hh)]
    if w > 0:
        px = py = 0
        while True:
            a, b = bits.u(h["b0"]), bits.u(h["b1"])
            while True:
                for on, count in ((0, a), (1, b)):
                    for _ in range(count):
                        if on and py < hh:
                            rows[py][px] = 1
                        px += 1
                        if px == w:
                            px, py = 0, py + 1
                if bits.u(1) == 0:
                    break
            if py >= hh:
                break
    return x, y, rows, dx


def decode_font(data):
    """Return (header, {codepoint: glyph}) for the 8-bit table and the unicode table."""
    h = parse_header(data)
    glyphs = {}
    pos = HEADER_SIZE
    while data[pos + 1] != 0:
        glyphs[data[pos]] = decode_glyph(h, Bits(data, pos + 2))
        pos += data[pos + 1]
    table = HEADER_SIZE + h["start_unicode"]
    if table + 4 <= len(data):
        pos = table + ((data[table] << 8) | data[table + 1])
        while pos + 3 <= len(data):
            code = (data[pos] << 8) | data[pos + 1]
            if code == 0:
                break
            glyphs[code] = decode_glyph(h, Bits(data, pos + 3))
            pos += data[pos + 2]
    return h, glyphs


def outline(x, y, rows):
    """Union of the lit pixels as clockwise contours (y up, font pixels); shared edges cancel."""
    edges = set()
    hh = len(rows)
    for r, row in enumerate(rows):
        for c, on in enumerate(row):
            if not on:
                continue
            px, py = x + c, y + hh - 1 - r
            corners = [(px, py), (px, py + 1), (px + 1, py + 1), (px + 1, py)]
            for i in range(4):
                e = (corners[i], corners[(i + 1) % 4])
                if (e[1], e[0]) in edges:
                    edges.remove((e[1], e[0]))
                else:
                    edges.add(e)
    out = {}
    for a, b in edges:
        out.setdefault(a, []).append(b)
    contours = []
    while out:
        start = next(iter(out))
        points, cur, prev = [start], start, None
        while True:
            ends = out[cur]
            nxt = ends[0]
            if len(ends) > 1 and prev is not None:  # two pixels touch at a corner: turn right to keep them apart
                d = (cur[0] - prev[0], cur[1] - prev[1])
                right = (cur[0] + d[1], cur[1] - d[0])
                nxt = right if right in ends else ends[0]
            ends.remove(nxt)
            if not ends:
                del out[cur]
            prev, cur = cur, nxt
            if cur == start:
                break
            points.append(cur)
        # Drop points in the middle of a straight run.
        n = len(points)
        keep = [p for i, p in enumerate(points)
                if (points[i - 1][0] - p[0]) * (points[(i + 1) % n][1] - p[1])
                != (points[i - 1][1] - p[1]) * (points[(i + 1) % n][0] - p[0])]
        contours.append(keep)
    return contours


def font_source(source):
    """Return (declared size, C string lines) of a U8g2 array: by name in u8g2_fonts.c, or the data[] of an ink_ui
    font file (components/core/ink_ui/fonts/*.c)."""
    if not source.endswith(".c"):
        return find_font(source)
    with open(source, encoding="latin-1") as f:
        text = f.read()
    m = re.search(r'data\[(\d+)\]\s*=((?:\s*"(?:[^"\\]|\\.)*")+)\s*;', text)  # ';' may occur inside the strings
    if not m:
        sys.exit(f"make_u8g2_ttf.py: không thấy mảng data[] trong {source}")
    return int(m.group(1)), re.findall(r'"(?:[^"\\]|\\.)*"', m.group(2))


def build(source, family, dst):
    size, lines = font_source(source)
    data = c_bytes(lines)
    if len(data) not in (size, size - 1):
        sys.exit(f"make_u8g2_ttf.py: đọc được {len(data)} byte, mảng khai báo {size}")
    h, glyphs = decode_font(data)
    # u8g2_SetFontRefHeightAll, as ink_ui_text.c uses.
    ascent = max(h["ascent_A"], h["max_h"] + h["y_offset"])
    descent = min(h["descent_g"], h["y_offset"])
    px_size = ascent - descent

    order = [".notdef"]
    cmap, ttglyphs, metrics = {}, {}, {}
    empty = TTGlyphPen(None).glyph()
    ttglyphs[".notdef"], metrics[".notdef"] = empty, (h["max_w"] * UNIT, 0)
    for code in sorted(glyphs):
        x, y, rows, dx = glyphs[code]
        name = f"uni{code:04X}"
        pen = TTGlyphPen(None)
        xmin = None
        for contour in outline(x, y, rows):
            pen.moveTo((contour[0][0] * UNIT, contour[0][1] * UNIT))
            for px, py in contour[1:]:
                pen.lineTo((px * UNIT, py * UNIT))
            pen.closePath()
            cx = min(p[0] for p in contour) * UNIT
            xmin = cx if xmin is None else min(xmin, cx)
        order.append(name)
        cmap[code] = name
        ttglyphs[name] = pen.glyph()
        metrics[name] = (dx * UNIT, xmin or 0)  # lsb = xMin, or rasterizers shift the glyph

    ps_name = re.sub(r"[^A-Za-z0-9-]", "", family) + "-Regular"
    fb = FontBuilder(px_size * UNIT, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)
    fb.setupGlyf(ttglyphs)
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=ascent * UNIT, descent=descent * UNIT, lineGap=0)
    fb.setupNameTable({"familyName": family, "styleName": "Regular", "fullName": f"{family} Regular",
                       "psName": ps_name, "version": "Version 1.0",
                       "uniqueFontIdentifier": f"{ps_name} from {os.path.basename(source)}"})
    fb.setupOS2(version=4, sTypoAscender=ascent * UNIT, sTypoDescender=descent * UNIT, sTypoLineGap=0,
                usWinAscent=ascent * UNIT, usWinDescent=-descent * UNIT, fsSelection=0x40 | 0x80)
    fb.setupPost()
    fb.font["head"].flags = 0x0001  # baseline at y = 0; lsb is not always 0
    fb.save(dst)
    return px_size, len(glyphs)


def main():
    sys.stdout.reconfigure(encoding="utf-8")  # Windows consoles may use a legacy code page
    parser = argparse.ArgumentParser(description="Đổi font U8g2 sang TTF cho Figma")
    parser.add_argument("u8g2_name", help="tên mảng font trong u8g2_fonts.c (ví dụ u8g2_font_6x10_tf), hoặc file font "
                                          "của ink_ui (ví dụ components/core/ink_ui/fonts/ink_ui_font_tiny5_8.c)")
    parser.add_argument("--family", required=True, help='tên font hiện trong Figma, ví dụ "Fixed 6x10"')
    parser.add_argument("-o", "--output", required=True)
    parser.add_argument("--install", action="store_true", help="cài vào thư mục font của người dùng (Windows)")
    args = parser.parse_args()
    px_size, count = build(args.u8g2_name, args.family, args.output)
    print(f"đã ghi {args.output}: {count} glyph; trong Figma đặt font '{args.family}', cỡ {px_size}, "
          f"line height {px_size}")
    if args.install:
        install(args.output, os.path.basename(args.output), f"{args.family} Regular (TrueType)")


if __name__ == "__main__":
    sys.exit(main())
