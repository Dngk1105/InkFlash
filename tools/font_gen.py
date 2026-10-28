# Thiết kế: docs/design/ui-canvas.md
"""Đổi một file TTF thành font 1 bit cho khối ink_ui: một cỡ chữ, ASCII và chữ tiếng Việt có dấu dựng sẵn.

    python tools/font_gen.py <file.ttf> --size 24 --name cascadia_code_24 --license "SIL OFL 1.1"

Sinh components/core/ink_ui/ink_ui_font_<name>.c và include/ink_ui_font_<name>.h (đổi thư mục bằng --out-dir),
rồi tự chạy clang-format để qua tools/check.py. Cần Pillow (pip install pillow).
"""
import argparse
import os
import struct
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DESIGN = "docs/design/ui-canvas.md"

VIETNAMESE = (
    "ÀÁÂÃÈÉÊÌÍÒÓÔÕÙÚÝàáâãèéêìíòóôõùúýĂăĐđĨĩŨũƠơƯư"
    "ẠạẢảẤấẦầẨẩẪẫẬậẮắẰằẲẳẴẵẶặẸẹẺẻẼẽẾếỀềỂểỄễỆệỈỉỊịỌọỎỏỐốỒồỔổỖỗỘộỚớỜờỞởỠỡỢợỤụỦủỨứỪừỬửỮữỰựỲỳỴỵỶỷỸỹ"
)
CHARSET = sorted({chr(c) for c in range(0x20, 0x7F)} | set(VIETNAMESE))


def font_notice(path):
    """Đọc dòng bản quyền (nameID 0) trong bảng name của TTF, để giữ thông báo bản quyền khi nhúng."""
    with open(path, "rb") as f:
        data = f.read()
    table_count = struct.unpack(">H", data[4:6])[0]
    for i in range(table_count):
        tag, _, offset, length = struct.unpack(">4sIII", data[12 + 16 * i:28 + 16 * i])
        if tag != b"name":
            continue
        table = data[offset:offset + length]
        count, string_offset = struct.unpack(">HH", table[2:6])
        for j in range(count):
            platform, _, _, name_id, size, start = struct.unpack(">HHHHHH", table[6 + 12 * j:18 + 12 * j])
            if name_id == 0:
                raw = table[string_offset + start:string_offset + start + size]
                return raw.decode("utf-16-be" if platform in (0, 3) else "latin-1")
    return ""


def render(font, char, threshold):
    """Trả (width, height, offset_x, offset_y, advance, rows) với rows là list các hàng bit (list int 0/1)."""
    advance = round(font.getlength(char))
    left, top, right, bottom = font.getbbox(char, anchor="ls")
    if right <= left or bottom <= top:
        return 0, 0, 0, 0, advance, []
    image = Image.new("L", (right - left, bottom - top), 0)
    ImageDraw.Draw(image).text((-left, -top), char, font=font, fill=255, anchor="ls")
    image = image.point(lambda v: 255 if v >= threshold else 0)
    box = image.getbbox()
    if box is None:
        return 0, 0, 0, 0, advance, []
    image = image.crop(box)
    width, height = image.size
    pixels = image.load()
    rows = [[1 if pixels[x, y] else 0 for x in range(width)] for y in range(height)]
    return width, height, left + box[0], top + box[1], advance, rows


def pack(rows, width):
    out = []
    for row in rows:
        for start in range(0, width, 8):
            byte = 0
            for bit in range(8):
                byte <<= 1
                if start + bit < width and row[start + bit]:
                    byte |= 1
            out.append(byte)
    return out


def find_clang_format():
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from check import find_clang_format as find  # noqa: E402
    return find()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("ttf")
    parser.add_argument("--size", type=int, required=True, help="cỡ chữ, tính bằng điểm ảnh")
    parser.add_argument("--name", required=True, help="tên font, ví dụ cascadia_code_24")
    parser.add_argument("--license", required=True, help="giấy phép của font, ghi vào đầu file sinh ra")
    parser.add_argument("--threshold", type=int, default=128, help="ngưỡng đổi xám sang đen trắng (mặc định 128)")
    parser.add_argument("--weight", type=float, help="độ đậm nét nếu là font biến thiên (mặc định theo font)")
    parser.add_argument("--out-dir", default=os.path.join(ROOT, "components", "core", "ink_ui"))
    parser.add_argument("--preview", action="store_true", help="in chữ mẫu ra terminal, không ghi file")
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")

    font = ImageFont.truetype(args.ttf, args.size)
    if args.weight is not None:
        font.set_variation_by_axes([args.weight])
    ascent, descent = font.getmetrics()

    notdef = font.getmask("￿")
    glyphs = []
    bitmaps = []
    for char in CHARSET:
        if char != " " and bytes(font.getmask(char)) == bytes(notdef):
            sys.exit(f"Font không có ký tự {char!r} (U+{ord(char):04X})")
        width, height, offset_x, offset_y, advance, rows = render(font, char, args.threshold)
        if not (-128 <= offset_x <= 127 and -128 <= offset_y <= 127 and advance <= 255):
            sys.exit(f"Số đo của {char!r} vượt khoảng lưu được trong 1 byte")
        glyphs.append((ord(char), len(bitmaps), width, height, offset_x, offset_y, advance, rows))
        bitmaps.extend(pack(rows, width))
    if ord("?") not in {g[0] for g in glyphs}:
        sys.exit("Thiếu ký tự '?'")
    if len(bitmaps) > 0xFFFF:
        sys.exit(f"Bitmap dài {len(bitmaps)} byte, vượt 65535")

    # Dòng đủ cao để không ký tự nào (kể cả chữ chồng dấu như Ậ) bị cắt khi (x, y) là góc trên trái của dòng.
    ascent = max(ascent, max(-g[5] for g in glyphs))
    descent = max(descent, max(g[5] + g[3] for g in glyphs))
    print(f"{len(glyphs)} ký tự, bitmap {len(bitmaps)} byte, ascent {ascent}, line_height {ascent + descent}")

    if args.preview:
        for char in "Hà Nội ẬẪ gjy":
            _, _, _, _, _, rows = render(font, char, args.threshold)
            for row in rows:
                print("".join("#" if v else "." for v in row))
            print()
        return

    symbol = f"ink_ui_font_{args.name}"
    notice = font_notice(args.ttf).replace("*/", "* /").strip()
    lines = [
        f"/* Thiết kế: {DESIGN} */",
        "/*",
        f" * Sinh bởi tools/font_gen.py từ {os.path.basename(args.ttf)}, cỡ {args.size}, giấy phép {args.license}.",
        f" * {notice}",
        " * Không sửa tay; sinh lại bằng font_gen.py.",
        " */",
        '#include "ink_ui.h"',
        f'#include "{symbol}.h"',
        "",
        "/* codepoint, bitmap_offset, width, height, offset_x, offset_y, advance */",
        "static const ink_ui_glyph_t s_glyphs[] = {",
    ]
    for cp, off, w, h, ox, oy, adv, _ in glyphs:
        lines.append(f"    {{0x{cp:04X}, {off}, {w}, {h}, {ox}, {oy}, {adv}}},")
    lines += ["};", "", "static const uint8_t s_bitmaps[] = {"]
    for start in range(0, len(bitmaps), 16):
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in bitmaps[start:start + 16]) + ",")
    lines += [
        "};", "",
        f"const ink_ui_font_t {symbol} = {{",
        "    .glyphs      = s_glyphs,",
        "    .bitmaps     = s_bitmaps,",
        f"    .glyph_count = {len(glyphs)},",
        f"    .line_height = {ascent + descent},",
        f"    .ascent      = {ascent},",
        "};", "",
    ]
    header = [
        f"/* Thiết kế: {DESIGN} */",
        "#pragma once",
        "",
        '#include "ink_ui.h"',
        "",
        f"extern const ink_ui_font_t {symbol};",
        "",
    ]
    os.makedirs(os.path.join(args.out_dir, "include"), exist_ok=True)
    paths = [(os.path.join(args.out_dir, symbol + ".c"), lines),
             (os.path.join(args.out_dir, "include", symbol + ".h"), header)]
    exe = find_clang_format()
    for path, content in paths:
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(content))
        if exe:
            subprocess.run([exe, "-i", f"--style=file:{os.path.join(ROOT, '.clang-format')}", path], check=True)
        print("Đã ghi", os.path.relpath(path, ROOT))
    if not exe:
        print("Không thấy clang-format: tự chạy clang-format -i trên hai file trên trước khi chạy tools/check.py")
    print(f"Thêm \"{symbol}.c\" vào SRCS trong CMakeLists.txt của ink_ui nếu chưa có.")


if __name__ == "__main__":
    main()
