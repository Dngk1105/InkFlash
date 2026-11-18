# Design: docs/design/figma-to-code.md
"""Công cụ sinh view: đọc một frame Figma, ghi <view>.gen.c, <view>.gen.h, <view>.sample.json.

    python tools/figma2ink.py <link Figma | thư mục snapshot> --app <app> [--out <thư mục>] [--save <thư mục>]

Link Figma: chuột phải frame → Copy link to selection. Cần biến môi trường FIGMA_TOKEN
(Personal access token, quyền File content: Read-only).
Thư mục snapshot: do --save ghi (node.json, icons/*.png, figma.png); đọc lại không cần mạng.
Tên frame phải là <app>_<màn>; view là <app>_<màn>_view. Mặc định ghi vào components/apps/ink_app_<app>/views/.
Lỗi theo bảng "Luật kiểm" thì dừng, không ghi file. Cảnh báo vẫn sinh và in ra.
"""
import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check import find_clang_format  # noqa: E402

API = "https://api.figma.com/v1"
DESIGN_LINE = "/* Design: docs/design/figma-to-code.md */"
HASH_RE = re.compile(r"^/\* Generated from Figma: .*; hash: (sha256:[0-9a-f]{16}|none) \*/$")
SIZES = {(240, 416): "portrait", (416, 240): "landscape"}

# Text style name in Figma → (ink_ui font symbol, line height px).
FONTS = {
    "unifont_vi_16": ("ink_ui_font_unifont_vi_16", 16),
    "fixed_10": ("ink_ui_font_fixed_10", 10),
}
ALIGN_H = {"LEFT": "INK_UI_ALIGN_START", "CENTER": "INK_UI_ALIGN_CENTER", "RIGHT": "INK_UI_ALIGN_END"}
ALIGN_AXIS = {"MIN": "INK_UI_ALIGN_START", "CENTER": "INK_UI_ALIGN_CENTER", "MAX": "INK_UI_ALIGN_END"}
CONTAINERS = {"FRAME", "GROUP", "COMPONENT", "COMPONENT_SET", "INSTANCE", "SECTION"}
VECTORS = {"VECTOR", "BOOLEAN_OPERATION", "ELLIPSE", "STAR", "POLYGON", "REGULAR_POLYGON"}
IDENT_RE = re.compile(r"[a-z][a-z0-9_]*")


# ---- Snapshot: REST API or a saved directory ----


def parse_link(link):
    """(file key, node id) from a figma.com link; node-id 1-2 in the URL is node 1:2."""
    url = urllib.parse.urlparse(link)
    m = re.match(r"/(?:design|file|proto)/([A-Za-z0-9]+)", url.path)
    node = urllib.parse.parse_qs(url.query).get("node-id", [None])[0]
    if not m or not node:
        fail("link Figma phải có dạng https://www.figma.com/design/<key>/...?node-id=<id> (Copy link to selection)")
    return m.group(1), node.replace("-", ":")


def api_get(path, token):
    req = urllib.request.Request(API + path, headers={"X-Figma-Token": token})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            return json.load(resp)
    except urllib.error.HTTPError as e:
        hint = " (token sai hoặc thiếu quyền File content)" if e.code in (401, 403) else ""
        fail(f"Figma API {e.code} khi gọi {path.split('?')[0]}{hint}")
    except urllib.error.URLError as e:
        fail(f"không gọi được Figma API: {e.reason}")


def download(url):
    with urllib.request.urlopen(url, timeout=60) as resp:
        return resp.read()


def fetch(link):
    """Snapshot dict from the REST API: node tree, styles, icon PNGs and a PNG of the whole frame."""
    token = os.environ.get("FIGMA_TOKEN")
    if not token:
        fail("chưa đặt FIGMA_TOKEN (Figma → Settings → Security → Personal access tokens, quyền File content: Read-only)")
    key, node_id = parse_link(link)
    data = api_get(f"/files/{key}/nodes?ids={urllib.parse.quote(node_id)}", token)
    entry = (data.get("nodes") or {}).get(node_id)
    if not entry:
        fail(f"file {key} không có node {node_id}")
    snap = {"file_key": key, "node_id": node_id, "name": data.get("name", ""), "version": data.get("version", ""),
            "document": entry["document"], "styles": entry.get("styles", {}), "icons": {}, "figma_png": None}
    icon_ids = [n["id"] for n in walk(entry["document"]) if n.get("type") in VECTORS and n.get("visible", True)]
    ids = [node_id] + icon_ids
    urls = api_get(f"/images/{key}?ids={urllib.parse.quote(','.join(ids))}&format=png&scale=1", token).get("images", {})
    snap["figma_png"] = download(urls[node_id]) if urls.get(node_id) else None
    for icon in icon_ids:
        if urls.get(icon):
            snap["icons"][icon] = download(urls[icon])
    return snap


def icon_file(node_id):
    return node_id.replace(":", "-").replace(";", "_") + ".png"


def save_snapshot(snap, path):
    os.makedirs(os.path.join(path, "icons"), exist_ok=True)
    meta = {k: snap[k] for k in ("file_key", "node_id", "name", "version", "document", "styles")}
    with open(os.path.join(path, "node.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(meta, f, ensure_ascii=False, indent=1)
    for node_id, png in snap["icons"].items():
        with open(os.path.join(path, "icons", icon_file(node_id)), "wb") as f:
            f.write(png)
    if snap["figma_png"]:
        with open(os.path.join(path, "figma.png"), "wb") as f:
            f.write(snap["figma_png"])


def load_snapshot(path):
    node_json = os.path.join(path, "node.json")
    if not os.path.isfile(node_json):
        fail(f"{path} không có node.json (thư mục snapshot do --save ghi)")
    with open(node_json, encoding="utf-8") as f:
        snap = json.load(f)
    snap["icons"] = {}
    for n in walk(snap["document"]):
        png = os.path.join(path, "icons", icon_file(n["id"]))
        if os.path.isfile(png):
            with open(png, "rb") as f:
                snap["icons"][n["id"]] = f.read()
    snap["figma_png"] = None
    return snap


def walk(node):
    yield node
    for child in node.get("children", []):
        yield from walk(child)


# ---- PNG → 1 bit mask (icons) ----


def png_alpha(png, luma=False):
    """(width, height, rows of alpha 0..255) of an 8 bit, non-interlaced PNG; no alpha channel = darkness.
    luma=True returns the luminance 0..255 instead (for comparing a Figma export with a render)."""
    if png[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("không phải PNG")
    pos, idat, ihdr = 8, b"", None
    while pos < len(png):
        length, kind = struct.unpack(">I4s", png[pos:pos + 8])
        body = png[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    width, height, depth, ctype, _, _, interlace = ihdr
    channels = {0: 1, 2: 3, 4: 2, 6: 4}.get(ctype)
    if depth != 8 or channels is None or interlace:
        raise ValueError("PNG phải 8 bit, không palette, không interlace")
    raw, stride, bpp = zlib.decompress(idat), width * channels, channels
    rows, prev = [], bytearray(stride)
    for y in range(height):
        ftype, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b, c = prev[i], prev[i - bpp] if i >= bpp else 0
            if ftype == 1:
                line[i] = (line[i] + a) & 0xFF
            elif ftype == 2:
                line[i] = (line[i] + b) & 0xFF
            elif ftype == 3:
                line[i] = (line[i] + (a + b) // 2) & 0xFF
            elif ftype == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        prev = line
        if luma:
            rows.append([line[x * channels] if channels < 3 else (299 * line[x * channels] + 587 * line[x * channels + 1]
                                                                  + 114 * line[x * channels + 2]) // 1000
                         for x in range(width)])
        elif channels in (2, 4):
            rows.append([line[x * channels + channels - 1] for x in range(width)])
        else:
            rows.append([255 - line[x * channels] for x in range(width)])
    return width, height, rows


def mask1(width, rows):
    """MASK1 bytes: alpha >= 128 is 1; rows padded to a byte, leftmost pixel in the MSB."""
    out = []
    for row in rows:
        for x0 in range(0, width, 8):
            out.append(sum(1 << (7 - b) for b in range(8) if x0 + b < width and row[x0 + b] >= 128))
    return out


# ---- Layout math, same integer arithmetic as ink_ui_layout.c ----


def l_inset(r, h, v):
    x, y, w, hh = r
    nw, nh = w - 2 * h, hh - 2 * v
    return (x + h if nw > 0 else x + int(w / 2), y + v if nh > 0 else y + int(hh / 2), max(nw, 0), max(nh, 0))


def l_split(r, column, gap, lengths):
    total = max(r[3] if column else r[2], 0)
    free = 0
    if total > 0:
        free = max(total - (gap * (len(lengths) - 1) if len(lengths) > 1 else 0) - sum(f for f, _ in lengths), 0)
    weights = sum(fl for _, fl in lengths)
    last = max((i for i, (_, fl) in enumerate(lengths) if fl > 0), default=len(lengths))
    out, offset, shared = [], 0, 0
    for i, (fixed, flex) in enumerate(lengths):
        share = int(free * flex / weights) if weights else 0
        if i == last:
            share = free - shared
        shared += share
        length = fixed + share
        if offset < 0:
            clamped, off = min(max(offset + length, 0), total), 0
        elif offset >= total:
            clamped, off = 0, total
        else:
            clamped, off = max(length if offset + length <= total else total - offset, 0), offset
        out.append((r[0], r[1] + off, max(r[2], 0), clamped) if column else (r[0] + off, r[1], clamped, max(r[3], 0)))
        offset += length + gap
    return out


def l_align(outer, size, ha, va):
    def off(o, s, a):
        return 0 if a == "INK_UI_ALIGN_START" else int((o - s) / 2) if a == "INK_UI_ALIGN_CENTER" else o - s

    return (outer[0] + off(outer[2], size[0], ha), outer[1] + off(outer[3], size[1], va), size[0], size[1])


# ---- Generator ----


def fail(msg):
    print(f"Lỗi: {msg}", file=sys.stderr)
    sys.exit(1)


def c_string(text):
    out = []
    for ch in text:
        if ch in '"\\':
            out.append("\\" + ch)
        elif ch == "\n":
            out.append("\\n")
        elif ord(ch) < 0x20:
            out.append(f"\\{ord(ch):03o}")
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


def ascii_name(node):
    """Layer name for a C comment; non-ASCII names are left out (comments are English)."""
    name = node.get("name", "")
    return name if name.isascii() and "*/" not in name else node.get("type", "").lower()


class Scope:
    """Where a node is drawn: top level of the view or inside one row of *rows."""

    def __init__(self, data_expr="data->", anchor=None, origin=(0, 0), fields=None):
        self.data_expr = data_expr  # C prefix of the fields: "data->" or "data->rows[i]."
        self.anchor = anchor  # C rect variable the literals are relative to (row rect), or None
        self.origin = origin  # absolute position of `anchor` in the template row
        self.fields = fields if fields is not None else []  # [(kind, name, extra)] of this struct


class Generator:
    def __init__(self, snap, app):
        self.snap = snap
        self.app = app
        self.errors = []
        self.warnings = []
        self.styles = {}  # (font, align, max_lines, line_gap) → C name
        self.icons = []  # lines of static icon data
        self.names = set()  # C locals and statics already used
        self.root = snap["document"]
        self.view = ""
        self.size = (0, 0)
        self.origin = (0, 0)
        self.bg = 255

    # -- messages --

    def where(self, node):
        return f"layer '{node.get('name', '?')}' ({node.get('id', '?')})"

    def error(self, node, msg):
        self.errors.append(f"{self.where(node)}: {msg}")

    def warn(self, node, msg):
        self.warnings.append(f"{self.where(node)}: {msg}")

    # -- geometry --

    def rect(self, node, key="absoluteBoundingBox"):
        box = node.get(key) or node.get("absoluteBoundingBox")
        if not box:
            self.error(node, "không có toạ độ")
            return (0, 0, 0, 0)
        vals = (box["x"] - self.origin[0], box["y"] - self.origin[1], box["width"], box["height"])
        if any(abs(v - round(v)) > 0.01 for v in vals):
            self.error(node, f"toạ độ hoặc cỡ không nguyên: {tuple(round(v, 2) for v in vals)}")
        return tuple(int(round(v)) for v in vals)

    def lit(self, r, scope):
        if scope.anchor:
            a, (ox, oy) = scope.anchor, scope.origin
            return (f"(ink_ui_rect_t){{.x = (int16_t)({a}.x + {r[0] - ox}), .y = (int16_t)({a}.y + {r[1] - oy}), "
                    f".width = {r[2]}, .height = {r[3]}}}")
        return f"(ink_ui_rect_t){{.x = {r[0]}, .y = {r[1]}, .width = {r[2]}, .height = {r[3]}}}"

    @staticmethod
    def coord(v, axis, scope):
        """C expression of an absolute x or y, relative to the row rect inside *rows."""
        if not scope.anchor:
            return str(v)
        return f"(int16_t)({scope.anchor}.{axis} + {v - scope.origin[0 if axis == 'x' else 1]})"

    def unique(self, base):
        base = re.sub(r"\W+", "_", base).strip("_").lower() or "node"
        if base[0].isdigit():
            base = "n_" + base
        name, i = base, 2
        while name in self.names:
            name, i = f"{base}_{i}", i + 1
        self.names.add(name)
        return name

    # -- paints --

    def solid(self, node, paints, what):
        """Gray level 0..255 of the single visible solid paint, None when there is none."""
        visible = [p for p in paints or [] if p.get("visible", True)]
        if not visible:
            return None
        if len(visible) > 1:
            self.error(node, f"{what} có nhiều lớp màu; giữ một")
            return None
        p = visible[0]
        if p.get("type") != "SOLID":
            self.error(node, f"{what} kiểu {p.get('type')}: chỉ nhận màu đặc (không gradient, không ảnh)")
            return None
        c = p.get("color", {})
        if p.get("opacity", 1) * c.get("a", 1) < 0.999:
            self.error(node, f"{what} có opacity dưới 100 %")
        r, g, b = (int(round(c.get(k, 0) * 255)) for k in "rgb")
        if not r == g == b:
            self.error(node, f"{what} màu ({r}, {g}, {b}) không phải xám (R = G = B)")
        return r

    @staticmethod
    def paint(level):
        return {0: "INK_UI_PAINT_BLACK", 255: "INK_UI_PAINT_WHITE"}.get(level, f"INK_UI_PAINT_GRAY({level})")

    def check_common(self, node):
        if node.get("opacity", 1) < 0.999:
            self.error(node, "opacity dưới 100 %")
        if any(e.get("visible", True) for e in node.get("effects", [])):
            self.error(node, "có effect (shadow, blur): panel 1 bit không vẽ được")

    def radius(self, node):
        radii = node.get("rectangleCornerRadii")
        if radii and len(set(radii)) > 1:
            self.error(node, "các góc bo khác nhau: chỉ nhận một corner radius")
        return int(round(node.get("cornerRadius", 0)))

    def box_paint(self, node, r, scope, out):
        """Fill and inside stroke of a rectangle or frame."""
        expr = self.lit(r, scope)
        rad = self.radius(node)
        fill = self.solid(node, node.get("fills"), "fill")
        if fill is not None:
            if fill not in (0, 255) and rad:
                self.warn(node, "fill xám có corner radius: fill dither bỏ qua radius")
            out.append(f"ink_ui_fill(canvas, {expr}, {rad}, {self.paint(fill)});")
        stroke = self.solid(node, node.get("strokes"), "stroke")
        weight = node.get("strokeWeight", 0)
        if stroke is not None and weight:
            if node.get("strokeAlign", "INSIDE") != "INSIDE":
                self.error(node, f"stroke canh {node.get('strokeAlign')}: ink_ui_stroke() chỉ vẽ vào trong")
            if node.get("individualStrokeWeights"):
                self.error(node, "stroke dày khác nhau theo từng edge: chưa hỗ trợ")
            if stroke not in (0, 255):
                self.warn(node, "stroke xám: thành đen hoặc trắng theo ngưỡng 50 %")
            out.append(f"ink_ui_stroke(canvas, {expr}, {int(round(weight))}, {rad}, {self.paint(stroke)});")

    # -- names --

    def dynamic(self, node):
        """(kind, name) for {name}, ?name, *name layers; (None, None) for static layers."""
        name = node.get("name", "")
        m = re.fullmatch(r"\{(.*)\}", name) or re.fullmatch(r"([?*])(.*)", name)
        if not m:
            return None, None
        kind, field = ("{", m.group(1)) if name.startswith("{") else (m.group(1), m.group(2))
        if not IDENT_RE.fullmatch(field):
            self.error(node, f"tên '{field}' không hợp lệ: chữ thường, số, '_', bắt đầu bằng chữ")
        if kind == "{" and node.get("type") != "TEXT":
            self.error(node, "layer {tên} phải là Text")
        if kind == "*" and node.get("layoutMode", "NONE") == "NONE":
            self.error(node, "layer *tên phải là frame Auto Layout chứa các hàng")
        return kind, field

    def add_field(self, scope, node, kind, name, extra=None):
        key = ("show_" + name) if kind == "?" else name
        if any(f[3] == key for f in scope.fields):
            self.error(node, f"trùng tên field '{key}'")
        scope.fields.append((kind, name, extra, key))

    def check_grid(self, node, r):
        if r[0] % 8 or (r[0] + r[2]) % 8:
            self.warn(node, f"mép trái/phải ({r[0]}, {r[0] + r[2]}) không trên bội của 8 px; "
                            "display căn changed window theo 8 px")

    # -- nodes --

    def node(self, node, scope, out, rect_expr=None):
        """Append C lines drawing `node`. rect_expr: C expression for its rect when a layout gives one."""
        kind, name = self.dynamic(node)
        visible = node.get("visible", True)
        if not visible and kind != "?":
            return
        r = self.rect(node)
        expr = rect_expr or self.lit(r, scope)
        if kind:
            self.check_grid(node, r)
        body = []
        if kind == "?":
            self.add_field(scope, node, "?", name, visible)
        self.check_common(node)
        t = node.get("type")
        if t == "TEXT":
            self.text(node, r, expr, scope, body, name if kind == "{" else None)
        elif t == "RECTANGLE":
            body.append(f"/* {ascii_name(node)}: rectangle {r[2]} x {r[3]} */")
            self.box_paint(node, r, scope, body)
        elif t == "LINE":
            self.line(node, scope, body)
        elif t in VECTORS:
            self.icon(node, scope, body)
        elif t in CONTAINERS:
            if kind == "*":
                self.rows(node, r, expr, scope, body, name)
            else:
                if t != "GROUP":
                    self.box_paint(node, r, scope, body)
                self.children(node, r, expr, scope, body)
        else:
            self.error(node, f"kiểu layer {t} chưa hỗ trợ")
        if kind == "?":
            out.append(f"if ({scope.data_expr}show_{name}) {{")
            out.extend(body)
            out.append("}")
        else:
            out.extend(body)

    def text(self, node, r, expr, scope, out, field):
        style = node.get("style", {})
        style_id = (node.get("styles") or {}).get("text")
        style_name = (self.snap["styles"].get(style_id) or {}).get("name") if style_id else None
        if style_name not in FONTS:
            self.error(node, f"text style '{style_name or '(không có)'}' không có trong font của ui: "
                             f"{', '.join(FONTS)}")
            return
        font, line_px = FONTS[style_name]
        if any(o for o in node.get("characterStyleOverrides", [])):
            self.error(node, "một text có nhiều style: tách thành nhiều layer")
        if abs(style.get("letterSpacing", 0)) > 0.01:
            self.error(node, "letter spacing khác 0")
        gap = 0
        if style.get("lineHeightUnit", "PIXELS") == "PIXELS" and "lineHeightPx" in style:
            gap = int(round(style["lineHeightPx"])) - line_px
        if style.get("textAlignVertical", "TOP") != "TOP" and r[3] != line_px:
            self.error(node, "căn dọc giữa/dưới: chỉ nhận căn trên (ink_ui_text_draw vẽ từ mép trên)")
        align = ALIGN_H.get(style.get("textAlignHorizontal", "LEFT"))
        if not align:
            self.error(node, "căn ngang justified: chưa hỗ trợ")
            return
        max_lines = int(style.get("maxLines", 0)) if style.get("textTruncation") == "ENDING" else 0
        level = self.solid(node, node.get("fills"), "màu chữ")
        if level is None:
            return
        if level not in (0, 255):
            self.error(node, "chữ màu xám: bị làm thành đen hoặc trắng theo ngưỡng 50 %")
        key = (style_name, align, max_lines, gap)
        if key not in self.styles:
            parts = ["s", style_name.split("_")[0], align.rsplit("_", 1)[1].lower()]
            if max_lines:
                parts.append(f"l{max_lines}")
            if gap:
                parts.append(f"g{gap}".replace("-", "m"))
            self.styles[key] = ("_".join(parts), font)
        sname = self.styles[key][0]
        if field:
            if style.get("textAutoResize", "NONE") not in ("NONE", "TRUNCATE"):
                self.error(node, "text {tên} để Hug/Auto height: đặt Fixed, vì cỡ chữ động không biết trước")
            self.add_field(scope, node, "{", field, node.get("characters", ""))
            text = f"{scope.data_expr}{field}"
            out.append(f"/* {{{field}}}: {style_name}, {r[2]} x {r[3]} */")
        else:
            text = c_string(node.get("characters", ""))
            out.append(f"/* {ascii_name(node)}: static text, {style_name} */")
        out.append(f"ink_ui_text_draw(canvas, {expr}, {text}, &{sname}, {self.paint(level)});")

    def line(self, node, scope, out):
        box = node.get("absoluteRenderBounds") or {}
        if any(abs(box.get(k, 0) - round(box.get(k, 0))) > 0.01 for k in ("x", "y", "width", "height")):
            self.error(node, "line vẽ lệch nửa pixel (stroke canh giữa đường); dùng Rectangle cao/rộng 1 px thay cho Line")
            return
        r = self.rect(node, "absoluteRenderBounds")
        level = self.solid(node, node.get("strokes"), "line")
        if level is None:
            return
        if level not in (0, 255):
            self.warn(node, "line xám: thành đen hoặc trắng theo ngưỡng 50 %")
        if r[2] > 1 and r[3] > 1 and node.get("strokeWeight", 1) < min(r[2], r[3]):
            self.error(node, "line chéo: chỉ nhận line ngang hoặc dọc")
            return
        horizontal = r[2] >= r[3]
        thick = r[3] if horizontal else r[2]
        if thick == 1:
            x0, y0 = r[0], r[1]
            x1, y1 = (r[0] + r[2] - 1, r[1]) if horizontal else (r[0], r[1] + r[3] - 1)
            out.append(f"/* {ascii_name(node)}: line, weight 1 */")
            out.append(f"ink_ui_line(canvas, {self.coord(x0, 'x', scope)}, {self.coord(y0, 'y', scope)}, "
                       f"{self.coord(x1, 'x', scope)}, {self.coord(y1, 'y', scope)}, 1, {self.paint(level)});")
        else:
            out.append(f"/* {ascii_name(node)}: line, weight {thick}, drawn as its painted rect */")
            out.append(f"ink_ui_fill(canvas, {self.lit(r, scope)}, 0, {self.paint(level)});")

    def icon(self, node, scope, out):
        png = self.snap["icons"].get(node["id"])
        if png is None:
            self.error(node, "thiếu ảnh PNG của icon trong snapshot (chạy lại từ link Figma)")
            return
        try:
            width, height, rows = png_alpha(png)
        except (ValueError, zlib.error, TypeError) as e:
            self.error(node, f"đọc PNG của icon lỗi: {e}")
            return
        level = self.solid(node, node.get("fills"), "fill icon")
        if level is None:
            level = self.solid(node, node.get("strokes"), "stroke icon")
        if level is None:
            return
        if level not in (0, 255):
            self.error(node, "icon xám: icon là ảnh 1 bit, chỉ đen hoặc trắng")
        r = self.rect(node, "absoluteRenderBounds")
        name = self.unique("s_icon_" + ascii_name(node))
        data = mask1(width, rows)
        hexes = ", ".join(f"0x{b:02X}" for b in data)
        self.icons.append(f"/* Icon {ascii_name(node)}: {width} x {height}, MASK1. */")
        self.icons.append(f"static const uint8_t {name}_data[{len(data)}] = {{{hexes}}};")
        self.icons.append(f"static const ink_ui_bitmap_t {name} = {{.width = {width}, .height = {height}, "
                          f".format = INK_UI_BITMAP_FORMAT_MASK1, .data = {name}_data}};")
        out.append(f"/* {ascii_name(node)}: icon {width} x {height} */")
        out.append(f"ink_ui_bitmap(canvas, {self.coord(r[0], 'x', scope)}, {self.coord(r[1], 'y', scope)}, &{name}, "
                   f"{self.paint(level)});")

    def children(self, node, r, expr, scope, out):
        kids = node.get("children", [])
        if node.get("layoutMode", "NONE") in ("HORIZONTAL", "VERTICAL") and node.get("type") != "GROUP":
            self.auto_layout(node, r, expr, scope, out, kids)
            return
        for child in kids:
            if child.get("name", "").startswith("part:"):
                self.error(child, "part:<tên> chỉ được là frame con cấp một của màn")
            self.node(child, scope, out)

    def flow(self, node, kids):
        """Children laid out by Auto Layout (visible, not absolute). Hidden ?layers would shift siblings."""
        flow = []
        for child in kids:
            if child.get("layoutPositioning") == "ABSOLUTE":
                continue
            if not child.get("visible", True):
                if child.get("name", "").startswith("?"):
                    self.error(child, "layer ?tên trong Auto Layout đang ẩn: Figma dồn các layer khác lại, view thì "
                                      "không; hiện layer này rồi chạy lại")
                continue
            flow.append(child)
        return flow

    def layout_plan(self, node, r, expr, flow):
        """(C lines, slot C names, emulated slot rects) of one Auto Layout frame."""
        column = node["layoutMode"] == "VERTICAL"
        if node.get("layoutWrap") == "WRAP":
            self.error(node, "Auto Layout wrap: chưa hỗ trợ")
        pl, pr = node.get("paddingLeft", 0), node.get("paddingRight", 0)
        pt, pb = node.get("paddingTop", 0), node.get("paddingBottom", 0)
        rects = [self.rect(c) for c in flow]
        main = 1 if column else 0
        lengths = []
        for c, cr in zip(flow, rects):
            sizing = c.get("layoutSizingVertical" if column else "layoutSizingHorizontal")
            fill = sizing == "FILL" or (sizing is None and c.get("layoutGrow", 0) == 1)
            lengths.append((0, 1) if fill else (cr[main + 2], 0))
        gap = int(round(node.get("itemSpacing", 0)))
        if len(rects) > 1:
            gaps = {rects[i + 1][main] - rects[i][main] - rects[i][main + 2] for i in range(len(rects) - 1)}
            if len(gaps) == 1:
                gap = gaps.pop()
        if pl == pr and pt == pb:
            inner_expr = f"ink_ui_rect_inset({expr}, {int(pl)}, {int(pt)})" if pl or pt else expr
            inner = l_inset(r, int(pl), int(pt))
        else:
            inner = (r[0] + int(pl), r[1] + int(pt), r[2] - int(pl + pr), r[3] - int(pt + pb))
            inner_expr = None
        if not any(fl for _, fl in lengths) and rects:
            # No Fill child: the children may sit centered or at the end; split only the span they cover.
            start = rects[0][main]
            span = rects[-1][main] + rects[-1][main + 2] - start
            if start != inner[main]:
                inner = (inner[0], start, inner[2], span) if column else (start, inner[1], span, inner[3])
                inner_expr = None
        return column, lengths, gap, inner, inner_expr, rects

    def auto_layout(self, node, r, expr, scope, out, kids):
        flow = self.flow(node, kids)
        if not flow:
            for child in kids:
                self.node(child, scope, out)
            return
        column, lengths, gap, inner, inner_expr, rects = self.layout_plan(node, r, expr, flow)
        base = self.unique(ascii_name(node))
        n = len(flow)
        slots = l_split(inner, column, gap, lengths)
        axis = "INK_UI_AXIS_COLUMN" if column else "INK_UI_AXIS_ROW"
        out.append(f"/* {ascii_name(node)}: Auto Layout {'column' if column else 'row'}, spacing {gap} */")
        out.append(f"const ink_ui_length_t {base}_lengths[{n}] = {{"
                   + ", ".join(f"{{.fixed = {f}, .flex = {fl}}}" for f, fl in lengths) + "};")
        out.append(f"ink_ui_rect_t {base}_slots[{n}];")
        out.append(f"ink_ui_rect_split({inner_expr or self.lit(inner, scope)}, {axis}, {gap}, {base}_lengths, {n}, "
                   f"{base}_slots);")
        for i, (child, cr) in enumerate(zip(flow, rects)):
            self.node(child, scope, out, self.slot_expr(node, cr, slots[i], f"{base}_slots[{i}]", column, scope))
        for child in kids:
            if child.get("layoutPositioning") == "ABSOLUTE":
                self.node(child, scope, out)

    def slot_expr(self, node, cr, slot, slot_c, column, scope):
        """C expression giving the child rect `cr` from its slot; a literal when the math does not land on it."""
        if slot == cr:
            return slot_c
        cross = ALIGN_AXIS.get(node.get("counterAxisAlignItems", "MIN"), "INK_UI_ALIGN_START")
        ha, va = ("INK_UI_ALIGN_START", cross) if not column else (cross, "INK_UI_ALIGN_START")
        if l_align(slot, (cr[2], cr[3]), ha, va) == cr:
            return f"ink_ui_rect_align({slot_c}, (ink_ui_size_t){{.width = {cr[2]}, .height = {cr[3]}}}, {ha}, {va})"
        return self.lit(cr, scope)

    def rows(self, node, r, expr, scope, out, name):
        """*name: Auto Layout of repeated rows; the first row is the template drawn for each data row."""
        if scope.anchor:
            self.error(node, "*tên lồng trong *tên khác: chưa hỗ trợ")
            return
        flow = self.flow(node, node.get("children", []))
        if not flow:
            self.error(node, "*tên không có hàng nào đang hiện")
            return
        self.box_paint(node, r, scope, out)
        column, lengths, gap, inner, inner_expr, rects = self.layout_plan(node, r, expr, flow)
        macro = f"{self.view.upper()}_{name.upper()}_MAX"
        n = len(flow)
        slots = l_split(inner, column, gap, lengths)
        axis = "INK_UI_AXIS_COLUMN" if column else "INK_UI_AXIS_ROW"
        row_scope = Scope(f"data->{name}[i].", "row", rects[0][:2], [])
        body = []
        self.node_children_as_row(flow[0], rects[0], row_scope, body)
        if not row_scope.fields:
            self.error(node, "hàng mẫu của *tên không có layer động ({tên} hoặc ?tên)")
        samples = []
        for row in flow:
            sample_scope = Scope(fields=[])
            self.collect(row, sample_scope)
            if [f[3] for f in sample_scope.fields] != [f[3] for f in row_scope.fields]:
                self.error(row, f"hàng của *{name} khác hàng mẫu (cùng tên layer động, cùng thứ tự)")
            samples.append({f[3]: f[2] for f in sample_scope.fields})
        self.add_field(scope, node, "*", name, {"macro": macro, "max": n, "fields": row_scope.fields,
                                                "samples": samples})
        out.append(f"/* *{name}: Auto Layout {'column' if column else 'row'}, spacing {gap}, {n} rows in Figma */")
        out.append(f"const ink_ui_length_t {name}_lengths[{macro}] = {{"
                   + ", ".join(f"{{.fixed = {f}, .flex = {fl}}}" for f, fl in lengths) + "};")
        out.append(f"ink_ui_rect_t {name}_slots[{macro}];")
        out.append(f"ink_ui_rect_split({inner_expr or self.lit(inner, scope)}, {axis}, {gap}, {name}_lengths, {macro}, "
                   f"{name}_slots);")
        out.append(f"const uint8_t {name}_count = {scope.data_expr}{name}_count < {macro} ? "
                   f"{scope.data_expr}{name}_count : {macro};")
        out.append(f"for (uint8_t i = 0; i < {name}_count; i++) {{")
        out.append(f"const ink_ui_rect_t row = {name}_slots[i];")
        out.extend(body)
        out.append("}")
        for i, (s, cr) in enumerate(zip(slots, rects)):
            if s != cr:
                self.error(flow[i], f"hàng {i} lệch: Figma {cr}, rect_split cho {s}; các hàng phải cùng cỡ, "
                                    "canh đầu")

    def node_children_as_row(self, row, rr, scope, out):
        """Draw the template row relative to the C variable `row`."""
        self.check_common(row)
        if row.get("type") in CONTAINERS and row.get("type") != "GROUP":
            self.box_paint(row, rr, scope, out)
        if row.get("type") in CONTAINERS:
            self.children(row, rr, "row", scope, out)
        else:
            self.node(row, scope, out, "row")

    def collect(self, node, scope):
        """Dynamic fields of a subtree and their Figma values, without emitting code."""
        kind, name = self.dynamic(node)
        if kind == "?":
            scope.fields.append(("?", name, node.get("visible", True), "show_" + name))
        if not node.get("visible", True):
            return
        if kind == "{":
            scope.fields.append(("{", name, node.get("characters", ""), name))
        for child in node.get("children", []):
            self.collect(child, scope)

    # -- whole view --

    def generate(self):
        root = self.root
        name = root.get("name", "")
        if root.get("type") not in ("FRAME", "COMPONENT", "INSTANCE"):
            self.error(root, "node đã chọn không phải frame của màn")
        if not IDENT_RE.fullmatch(name) or not name.startswith(self.app + "_") or name == self.app + "_":
            self.error(root, f"tên frame phải là {self.app}_<màn> (chữ thường, số, '_'), ví dụ {self.app}_list")
        self.view = name + "_view"
        box = root.get("absoluteBoundingBox", {})
        self.origin = (box.get("x", 0), box.get("y", 0))
        self.size = (int(round(box.get("width", 0))), int(round(box.get("height", 0))))
        if self.size not in SIZES:
            self.error(root, f"frame cỡ {self.size[0]} x {self.size[1]}: màn phải 240 x 416 (dọc) hoặc 416 x 240 "
                             "(ngang)")
        self.check_common(root)
        bg = self.solid(root, root.get("fills"), "fill của màn")
        bg = 255 if bg is None else bg
        if bg not in (0, 255):
            self.error(root, "fill của màn phải đen hoặc trắng")
        self.bg = bg

        top = Scope()
        parts, main, statics = [], [], []
        for child in root.get("children", []):
            cname = child.get("name", "")
            if not cname.startswith("part:"):
                self.node(child, top, main)
                continue
            part = cname[len("part:"):]
            if not IDENT_RE.fullmatch(part):
                self.error(child, f"tên part '{part}' không hợp lệ")
                continue
            if child.get("type") not in CONTAINERS - {"GROUP"}:
                self.error(child, "part:<tên> phải là frame")
                continue
            if not child.get("visible", True):
                self.error(child, "part đang ẩn")
                continue
            r = self.rect(child)
            sname = self.unique("s_" + part)
            statics.append(f"static const ink_ui_rect_t {sname} = {{.x = {r[0]}, .y = {r[1]}, .width = {r[2]}, "
                           f".height = {r[3]}}};")
            body = []
            self.check_common(child)
            fill = self.solid(child, child.get("fills"), "fill")
            if fill is None:
                body.append(f"ink_ui_fill(canvas, {sname}, 0, {self.paint(bg)}); /* part background: screen fill */")
            else:
                body.append(f"ink_ui_fill(canvas, {sname}, {self.radius(child)}, {self.paint(fill)});")
            child_copy = dict(child, fills=[])  # background drawn above
            stroke_only = []
            self.box_paint(child_copy, r, top, stroke_only)
            body.extend(stroke_only)
            self.children(child, r, sname, top, body)
            parts.append((part, body))
            main.append(f"{self.view}_draw_{part}(canvas, data);")
        return top, parts, main, statics, bg

    # -- files --

    def header_text(self, top, parts):
        v, V = self.view, self.view.upper()
        lines = [
            "/* Generated from Figma: HASH_LINE */",
            DESIGN_LINE,
            "#pragma once",
            "",
            f"/* View {v}: Figma frame {self.root['name']}. Do not edit; edit Figma and run tools/figma2ink.py again. */",
            '#include "ink_ui.h"',
            "#include <stdint.h>",
            "",
            f"#define {V}_WIDTH  {self.size[0]} /**< canvas width after rotation, px */",
            f"#define {V}_HEIGHT {self.size[1]} /**< canvas height after rotation, px */",
        ]
        rows = [f for f in top.fields if f[0] == "*"]
        for f in rows:
            lines.append(f"#define {f[2]['macro']} {f[2]['max']} /**< rows drawn in Figma for *{f[1]} */")
        lines.append("")
        for f in rows:
            lines.append(f"/** One row of *{f[1]}. */")
            lines.append("typedef struct {")
            lines.extend(self.struct_fields(f[2]["fields"]))
            lines.append(f"}} {v}_{f[1]}_row_t;")
            lines.append("")
        lines.append("/** Dynamic data of the screen, one field per named layer. */")
        lines.append("typedef struct {")
        if not top.fields:
            lines.append("    uint8_t unused; /**< the screen has no dynamic layer */")
        lines.extend(self.struct_fields(top.fields))
        lines.append(f"}} {v}_data_t;")
        lines.append("")
        lines.append("/** @brief Draw the whole screen. */")
        lines.append(f"void {v}_draw(ink_ui_canvas_t *canvas, const {v}_data_t *data);")
        for part, _ in parts:
            lines.append("")
            lines.append(f"/** @brief Draw part:{part} only (its own background included). */")
            lines.append(f"void {v}_draw_{part}(ink_ui_canvas_t *canvas, const {v}_data_t *data);")
        return "\n".join(lines) + "\n"

    def struct_fields(self, fields):
        out = []
        for kind, name, extra, key in fields:
            if kind == "{":
                out.append(f"    const char *{key}; /**< {{{name}}} */")
            elif kind == "?":
                out.append(f"    bool {key}; /**< ?{name} */")
            else:
                out.append(f"    {self.view}_{name}_row_t {name}[{extra['macro']}]; /**< *{name} */")
                out.append(f"    uint8_t {name}_count; /**< rows used in `{name}` */")
        return out

    def source_text(self, parts, main, statics):
        v = self.view
        lines = ["/* Generated from Figma: HASH_LINE */", DESIGN_LINE, f'#include "{v}.gen.h"', ""]
        if statics:
            lines.append(f"/* Parts of the {self.size[0]} x {self.size[1]} screen. */")
            lines.extend(statics)
            lines.append("")
        for (_, align, max_lines, gap), (sname, font) in sorted(self.styles.items(), key=lambda kv: kv[1][0]):
            lines.append(f"static const ink_ui_text_style_t {sname} = {{.font = &{font}, .align = {align}, "
                         f".max_lines = {max_lines}, .line_gap = {gap}}};")
        if self.styles:
            lines.append("")
        if self.icons:
            lines.extend(self.icons)
            lines.append("")
        for part, body in parts:
            lines.append(f"void {v}_draw_{part}(ink_ui_canvas_t *canvas, const {v}_data_t *data) {{")
            if not any("data->" in b for b in body):
                lines.append("    (void)data;")
            lines.extend("    " + b for b in body)
            lines.append("}")
            lines.append("")
        lines.append(f"void {v}_draw(ink_ui_canvas_t *canvas, const {v}_data_t *data) {{")
        if not any("data" in b for b in main):
            lines.append("    (void)data;")
        lines.append(f"    ink_ui_fill(canvas, ink_ui_canvas_bounds(canvas), 0, {self.paint(self.bg)});")
        lines.extend("    " + b for b in main)
        lines.append("}")
        return "\n".join(lines) + "\n"

    def sample(self, top):
        out = {}
        for kind, name, extra, key in top.fields:
            if kind == "*":
                out[name] = extra["samples"]
            else:
                out[key] = extra
        return out


def finish_file(path, text, source_line):
    """Write, clang-format, then put the hash of everything after line 1 into line 1."""
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    exe = find_clang_format()
    if not exe:
        fail("không tìm thấy clang-format (có sẵn trong C:\\Espressif\\tools\\esp-clang)")
    r = subprocess.run([exe, "-i", f"--style=file:{os.path.join(ROOT, '.clang-format')}", path],
                       capture_output=True, text=True)
    if r.returncode != 0:
        fail(f"clang-format lỗi trên {path}: {r.stderr}")
    with open(path, encoding="utf-8") as f:
        body = f.read().split("\n", 1)[1]
    line1 = f"/* Generated from Figma: {source_line}; hash: {file_hash(body)} */"
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(line1 + "\n" + body)


def file_hash(body):
    """Hash of a generated file from line 2 on: sha256, first 16 hex digits."""
    return "sha256:" + hashlib.sha256(body.encode("utf-8")).hexdigest()[:16]


def check_generated(path, text):
    """Error message when a .gen.c/.gen.h was edited by hand (hash mismatch), else None. tools/check.py calls this."""
    first, _, body = text.partition("\n")
    m = HASH_RE.match(first)
    if not m:
        return "dòng 1 phải là '/* Generated from Figma: ...; hash: sha256:<16 hex> */'"
    if m.group(1) == "none":
        return None  # hand-written sample (template app)
    if file_hash(body) != m.group(1):
        return "file sinh từ Figma đã bị sửa tay (hash lệch); sửa Figma rồi chạy lại tools/figma2ink.py"
    return None


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description="Sinh view .gen.c/.gen.h/.sample.json từ một frame Figma")
    parser.add_argument("source", help="link Figma tới frame của màn, hoặc thư mục snapshot")
    parser.add_argument("--app", required=True, help="tên app (thư mục components/apps/ink_app_<app>)")
    parser.add_argument("--out", help="thư mục ghi view; mặc định components/apps/ink_app_<app>/views")
    parser.add_argument("--save", help="ghi snapshot (node.json, icons, figma.png) vào thư mục này")
    args = parser.parse_args()

    if not IDENT_RE.fullmatch(args.app):
        fail("--app: chữ thường, số, '_'")
    out_dir = args.out
    if not out_dir:
        app_dir = os.path.join(ROOT, "components", "apps", f"ink_app_{args.app}")
        if not os.path.isdir(app_dir):
            fail(f"chưa có {os.path.relpath(app_dir, ROOT)}: tạo app bằng tools/new.py hoặc truyền --out")
        out_dir = os.path.join(app_dir, "views")

    if re.match(r"https?://", args.source):
        snap = fetch(args.source)
    else:
        snap = load_snapshot(args.source)
    if args.save:
        save_snapshot(snap, args.save)
        print(f"Đã lưu snapshot → {args.save}")

    gen = Generator(snap, args.app)
    top, parts, main_lines, statics, _ = gen.generate()
    for w in gen.warnings:
        print(f"Cảnh báo: {w}")
    if gen.errors:
        for e in gen.errors:
            print(f"Lỗi: {e}", file=sys.stderr)
        fail(f"{len(gen.errors)} lỗi; không ghi file")

    os.makedirs(out_dir, exist_ok=True)
    source_line = f"{snap.get('file_key', '?')} node {snap.get('node_id', '?')} (version {snap.get('version', '?')})"
    base = os.path.join(out_dir, gen.view)
    finish_file(base + ".gen.h", gen.header_text(top, parts), source_line)
    finish_file(base + ".gen.c", gen.source_text(parts, main_lines, statics), source_line)
    with open(base + ".sample.json", "w", encoding="utf-8", newline="\n") as f:
        json.dump(gen.sample(top), f, ensure_ascii=False, indent=2)
        f.write("\n")
    print(f"Đã sinh {gen.view} ({len(parts)} part) → {os.path.relpath(base, ROOT)}.gen.c/.gen.h/.sample.json")


if __name__ == "__main__":
    main()
