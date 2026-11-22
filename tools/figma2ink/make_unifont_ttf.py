# Design: docs/design/figma-to-code.md
"""Đổi GNU Unifont OTF (CFF) thành TTF (glyf) để cài vào máy và dùng trong Figma.

Figma vẽ chữ bằng TrueType, ink_ui vẽ bằng font U8g2 (xem components/core/ink_ui/fonts). Để chữ
trong Figma rơi đúng pixel mà màn hình sẽ vẽ, mỗi glyph phải nằm đúng chỗ so với gốc của nó:
cột trái của glyph (xMin) phải bằng left side bearing (lsb) trong bảng hmtx. Nếu hmtx ghi lsb = 0
nhưng glyph bắt đầu ở xMin = 4 (1 pixel của Unifont), rasterizer sẽ dịch glyph sang trái 1 pixel.
Script này tính lại lsb từ xMin để tránh lỗi đó.

Cách dùng (cần fontTools; tải OTF từ https://unifoundry.com/unifont/):
    python tools/figma2ink/make_unifont_ttf.py unifont-18.0.01.otf -o unifont-vi-18.0.01.ttf --install

--install chép file vào thư mục font của người dùng, đăng ký với Windows (AddFontResourceW và
WM_FONTCHANGE). Sau đó khởi động lại figma_agent.exe để Figma thấy font mới.
"""
import argparse
import os
import shutil
import sys


from fontTools import subset
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont, newTable

# Latin + Vietnamese subset: Basic Latin..Latin Extended-B, combining marks, Latin Extended Additional,
# general punctuation, dong sign, arrows.
RANGES = [(0x20, 0x24F), (0x300, 0x36F), (0x1E00, 0x1EFF), (0x2000, 0x206F), (0x20AB, 0x20AB), (0x2190, 0x2193)]
INSTALL_NAME = "unifont-vi-18.0.01.ttf"


def to_truetype(font):
    """Replace the CFF table by glyf/loca (Unifont outlines are straight lines, so conversion is exact)."""
    order = font.getGlyphOrder()
    glyph_set = font.getGlyphSet()
    glyf = newTable("glyf")
    glyf.glyphOrder = order
    glyf.glyphs = {}
    for name in order:
        pen = TTGlyphPen(glyph_set)
        glyph_set[name].draw(Cu2QuPen(pen, 1.0, reverse_direction=True))
        glyf.glyphs[name] = pen.glyph()
    font["loca"] = newTable("loca")
    font["glyf"] = glyf
    del font["CFF "]
    if "VORG" in font:
        del font["VORG"]
    maxp = font["maxp"]
    maxp.tableVersion = 0x00010000
    # Real values of the point/contour counts are recomputed when the table is compiled.
    for attr in ("maxPoints", "maxContours", "maxCompositePoints", "maxCompositeContours", "maxTwilightPoints",
                 "maxStorage", "maxFunctionDefs", "maxInstructionDefs", "maxStackElements", "maxSizeOfInstructions",
                 "maxComponentElements", "maxComponentDepth"):
        setattr(maxp, attr, 0)
    maxp.maxZones = 1
    font["head"].glyphDataFormat = 0
    font["post"].formatType = 2.0
    font["post"].extraNames = []
    font["post"].mapping = {}
    font.sfntVersion = "\x00\x01\x00\x00"


def fix_bearings(font):
    """Set hmtx lsb = glyph xMin for every glyph, so no rasterizer shifts the outline."""
    glyf = font["glyf"]
    hmtx = font["hmtx"]
    for name in font.getGlyphOrder():
        glyph = glyf[name]
        glyph.recalcBounds(glyf)
        advance, _ = hmtx[name]
        hmtx[name] = (advance, getattr(glyph, "xMin", 0) if glyph.numberOfContours else 0)


def build(src, dst):
    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.notdef_outline = True
    options.glyph_names = True
    font = subset.load_font(src, options)
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=[c for lo, hi in RANGES for c in range(lo, hi + 1)])
    subsetter.subset(font)
    to_truetype(font)
    fix_bearings(font)
    font.save(dst)


def install(path, file_name=INSTALL_NAME, registry_name="Unifont Regular (TrueType)"):
    """Copy to the per-user font folder, register it with GDI and in HKCU (kept after a reboot), tell running apps
    (Windows only)."""
    import ctypes
    import winreg
    target = os.path.join(os.environ["LOCALAPPDATA"], "Microsoft", "Windows", "Fonts", file_name)
    gdi32 = ctypes.windll.gdi32
    while os.path.exists(target) and gdi32.RemoveFontResourceW(target):  # release the copy Windows has loaded
        pass
    try:
        shutil.copyfile(path, target)
    except PermissionError:
        sys.exit(f"{target} đang bị một ứng dụng giữ; đóng Figma và figma_agent.exe rồi chạy lại")
    gdi32.AddFontResourceW(target)
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows NT\CurrentVersion\Fonts", 0,
                        winreg.KEY_SET_VALUE) as key:
        winreg.SetValueEx(key, registry_name, 0, winreg.REG_SZ, target)
    ctypes.windll.user32.SendMessageTimeoutW(0xFFFF, 0x001D, 0, 0, 2, 1000, None)  # WM_FONTCHANGE
    print(f"đã cài {target}; khởi động lại figma_agent.exe")


def main():
    sys.stdout.reconfigure(encoding="utf-8")  # Windows consoles may use a legacy code page
    parser = argparse.ArgumentParser(description="Đổi Unifont OTF sang TTF cho Figma")
    parser.add_argument("otf")
    parser.add_argument("-o", "--output", default=INSTALL_NAME)
    parser.add_argument("--install", action="store_true", help="cài vào thư mục font của người dùng (Windows)")
    args = parser.parse_args()
    build(args.otf, args.output)
    print(f"đã ghi {args.output}")
    if args.install:
        install(args.output)


if __name__ == "__main__":
    sys.exit(main())
