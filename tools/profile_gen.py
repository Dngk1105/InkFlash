# Thiết kế: docs/design/display/display.md
"""Kiểm và sinh danh sách profile bảng sóng của driver UC8253.

Đọc components/drivers/ink_drv_uc8253/profiles/profiles.json và bản dump OTP cạnh nó, kiểm luật của mục
"Kiểm tra profile", rồi sinh ink_drv_uc8253_profiles.c và include/ink_drv_uc8253_profiles.h.

  python tools/profile_gen.py                       kiểm và sinh file C
  python tools/profile_gen.py --check               chỉ kiểm, báo lỗi nếu file C đã sinh khác nguồn
  python tools/profile_gen.py --blob clean_edge -o clean_edge.inkw
                                                    gói một profile thành .inkw cho gói PROFILE_LOAD của bench
  python tools/profile_gen.py --list                in id, tên, nguồn, thời gian một lần vẽ
"""
import argparse
import json
import os
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DRIVER_DIR = os.path.join(ROOT, "components", "drivers", "ink_drv_uc8253")
SOURCE_PATH = os.path.join(DRIVER_DIR, "profiles", "profiles.json")
C_PATH = os.path.join(DRIVER_DIR, "ink_drv_uc8253_profiles.c")
H_PATH = os.path.join(DRIVER_DIR, "include", "ink_drv_uc8253_profiles.h")

# ---- Bố cục OTP và LUT, chế độ KW (UC8253 tr.22–24, 47–52; docs/hardware/parts/weact-epaper-3.7/waveform.md) ----

OTP_TR_FIRST = 0x2A
TR_BYTES = 0xE9
TR_COUNT = 9
TR_FRAME_RATE, TR_VG, TR_VSH, TR_VSL, TR_VDHR, TR_VCOM, TR_EOPT, TR_XON_LOW, TR_XON_HIGH = range(9)
TR_LUT_FIRST = 0x09

TABLE_NAMES = ["VCOM", "WW", "KW", "WK", "KK"]  # thứ tự lệnh 0x20–0x24
TABLE_BYTES = 42
GROUP_BYTES = 7
GROUP_COUNT = 6
MAX_PHASE_FRAMES = 63
LEVELS = {"0": 0, "VSH": 1, "VSL": 2, "VDHR": 3}

PURPOSES = ["text", "ui", "page", "clean_strong", "clean_fast", "clean_edge_center", "clean_edge_neighbor", "clean_light"]
SOURCE_HAND = 0xFF
NAME_BYTES = 16

# ---- Gói .inkw ----

BLOB_MAGIC = b"INKW"
BLOB_VERSION = 1


class ProfileError(Exception):
    pass


def pll_hz(code):
    """Bảng tần số khung của PLL (UC8253 tr.26)."""
    code &= 0x1F
    if code < 16:
        return 5 + 5 * code
    if code < 24:
        return 85 + 5 * (code - 16)
    return 130 + 10 * (code - 24)


def pll_code(hz):
    for code in range(32):
        if pll_hz(code) == hz:
            return code
    raise ProfileError(f"{hz} Hz không có trong bảng PLL")


def read_otp(path):
    data = []
    with open(path, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            data.extend(int(item, 16) for item in line.split())
    need = OTP_TR_FIRST + TR_COUNT * TR_BYTES
    if len(data) < need or data[0] != 0xA5:
        raise ProfileError(f"{path}: bản dump thiếu (cần {need} byte, khoá A5 ở byte đầu)")
    return data


def band_bytes(otp, name):
    if not (name.startswith("TR") and name[2:].isdigit() and int(name[2:]) < TR_COUNT):
        raise ProfileError(f"dải {name} không có; dùng TR0–TR{TR_COUNT - 1}")
    index = int(name[2:])
    first = OTP_TR_FIRST + index * TR_BYTES
    return index, otp[first:first + TR_BYTES]


# ---- Bảng LUT ----


def repeat_count(value):
    return 1 if value == 0 else value


def table_phases(table):
    """(mức, số khung đã nhân lặp) của từng pha. profiles/waveform_view.html đếm giống hệt."""
    phases = []
    for group in range(GROUP_COUNT):
        bytes_ = table[group * GROUP_BYTES:(group + 1) * GROUP_BYTES]
        group_repeat = repeat_count(bytes_[0])
        for i in range(4):
            phase = bytes_[1 + i]
            state_repeat = repeat_count(bytes_[5] if i < 2 else bytes_[6])
            phases.append((phase >> 6, (phase & 0x3F) * state_repeat * group_repeat))
    return phases


def table_frames(table):
    return sum(frames for _, frames in table_phases(table))


def table_charge(table):
    charge = 0
    for level, frames in table_phases(table):
        if level == LEVELS["VSH"]:
            charge += frames
        elif level == LEVELS["VSL"]:
            charge -= frames
    return charge


def pack_table(phases, name):
    """Các pha [mức, số khung] → 42 byte: 4 pha mỗi nhóm, lặp nhóm và lặp trạng thái bằng 1."""
    flat = []
    for level_name, frames in phases:
        if level_name not in LEVELS:
            raise ProfileError(f"bảng {name}: mức {level_name} không có; dùng 0, VSH, VSL")
        if level_name == "VDHR" and name != "VCOM":
            raise ProfileError(f"bảng {name}: không dùng mức VDHR trong bảng nguồn")
        if frames < 0:
            raise ProfileError(f"bảng {name}: số khung âm")
        while frames > MAX_PHASE_FRAMES:
            flat.append((LEVELS[level_name], MAX_PHASE_FRAMES))
            frames -= MAX_PHASE_FRAMES
        if frames > 0:
            flat.append((LEVELS[level_name], frames))
    if len(flat) > GROUP_COUNT * 4:
        raise ProfileError(f"bảng {name}: {len(flat)} pha, tối đa {GROUP_COUNT * 4}")
    table = [0] * TABLE_BYTES
    for group in range((len(flat) + 3) // 4):
        base = group * GROUP_BYTES
        table[base] = 1
        for i, (level, frames) in enumerate(flat[group * 4:group * 4 + 4]):
            table[base + 1 + i] = (level << 6) | frames
        table[base + 5] = 1
        table[base + 6] = 1
    return table


def drive_tables(frames, hold):
    """Điểm đổi màu kéo `frames` khung; điểm giữ màu đứng yên rồi kéo lại `hold` khung."""
    if frames <= 0 or hold < 0 or hold > frames:
        raise ProfileError(f"drive_frames {frames}, hold_frames {hold} sai")
    return {
        "VCOM": [["0", frames]],
        "WW": [["0", frames - hold], ["VSL", hold]],
        "KW": [["VSL", frames]],
        "WK": [["VSH", frames]],
        "KK": [["0", frames - hold], ["VSH", hold]],
    }


# ---- Dựng profile ----


def build_profile(spec, otp):
    name = spec["name"]
    if len(name.encode()) >= NAME_BYTES:
        raise ProfileError(f"{name}: tên dài quá {NAME_BYTES - 1} byte")
    band_name = spec.get("copy") or spec.get("base")
    if band_name is None or ("copy" in spec) == ("base" in spec):
        raise ProfileError(f"{name}: cần đúng một trong copy, base")
    band_index, band = band_bytes(otp, band_name)
    tables = [band[TR_LUT_FIRST + i * TABLE_BYTES:TR_LUT_FIRST + (i + 1) * TABLE_BYTES] for i in range(len(TABLE_NAMES))]
    unchanged_copy = "copy" in spec and "tables" not in spec and "frame_rate_hz" not in spec

    if "base" in spec:
        phases = drive_tables(spec["drive_frames"], spec.get("hold_frames", 0)) if "drive_frames" in spec else {}
        phases.update(spec.get("tables", {}))
        tables = [pack_table(phases.get(table_name, []), table_name) for table_name in TABLE_NAMES]
    elif "tables" in spec:
        for table_name, table_phases_spec in spec["tables"].items():
            if table_name not in TABLE_NAMES:
                raise ProfileError(f"{name}: bảng {table_name} không có")
            tables[TABLE_NAMES.index(table_name)] = pack_table(table_phases_spec, table_name)

    longest = max(table_frames(table) for table in tables[1:])
    if "base" in spec and table_frames(tables[0]) < longest:
        tables[0] = pack_table([["0", longest]], "VCOM")  # VCOM đứng yên suốt lần quét

    pll = pll_code(spec["frame_rate_hz"]) if "frame_rate_hz" in spec else band[TR_FRAME_RATE]
    scans = spec.get("scans", 1)
    if not 1 <= scans <= 4:
        raise ProfileError(f"{name}: scans {scans} ngoài 1–4")
    frames = max(table_frames(table) for table in tables)
    drf_ms = round((frames + 2) * 1000 / pll_hz(pll))
    return {
        "id": spec["id"],
        "name": name,
        "source": band_index if unchanged_copy else SOURCE_HAND,
        "copied": "copy" in spec,
        "allow_unbalanced": spec.get("allow_unbalanced", False),
        "scans": scans,
        "pll": pll,
        "power": [band[TR_VG], band[TR_VSH], band[TR_VSL], band[TR_VDHR]],
        "vcom": band[TR_VCOM],
        "eopt": band[TR_EOPT],
        "xon": [band[TR_XON_LOW], band[TR_XON_HIGH]],
        "drf_ms": drf_ms,
        "draw_ms": drf_ms * scans,
        "tables": [list(table) for table in tables],
    }


def check_profile(profile, warnings):
    """Luật của mục "Kiểm tra profile". Profile chép từ OTP (không sửa bảng) không kiểm cân bằng."""
    name = profile["name"]
    tables = dict(zip(TABLE_NAMES, profile["tables"]))
    if profile["copied"] and profile["source"] != SOURCE_HAND:
        return
    # chép OTP rồi sửa bảng: LUTWK, LUTKW vẫn của hãng nên không bắt cân bằng
    cycle = table_charge(tables["WK"]) + table_charge(tables["KW"])
    if cycle != 0 and not profile["copied"] and not profile["allow_unbalanced"]:
        raise ProfileError(f"{name}: LUTWK + LUTKW = {cycle} khung, phải bằng 0 (hoặc allow_unbalanced)")
    for hold in ("WW", "KK"):
        charge = table_charge(tables[hold])
        if charge != 0:
            warnings.append(f"{name}: bảng giữ màu {hold} lệch {charge} khung")


def build_all(source):
    otp = read_otp(os.path.join(os.path.dirname(SOURCE_PATH), source["otp"]))
    retired = set(source.get("retired_ids", []))
    profiles, warnings, ids = [], [], set()
    for spec in source["profiles"]:
        profile = build_profile(spec, otp)
        if not 1 <= profile["id"] <= 0xFFFE:
            raise ProfileError(f"{profile['name']}: id ngoài 1–65534")
        if profile["id"] in ids or profile["id"] in retired:
            raise ProfileError(f"{profile['name']}: id {profile['id']} trùng hoặc đã bỏ")
        ids.add(profile["id"])
        check_profile(profile, warnings)
        profiles.append(profile)
    purposes = []
    for purpose in PURPOSES:
        spec = source["purposes"].get(purpose)
        if spec is None:
            raise ProfileError(f"thiếu mục đích {purpose}")
        if not 1 <= len(spec["bands"]) <= 9:
            raise ProfileError(f"{purpose}: cần 1–9 dải")
        for max_celsius, profile_id in spec["bands"]:
            if profile_id not in ids:
                raise ProfileError(f"{purpose}: id {profile_id} không có")
            if not -128 <= max_celsius <= 127:
                raise ProfileError(f"{purpose}: {max_celsius} °C ngoài int8")
        purposes.append({"name": purpose, **spec})
    return profiles, purposes, warnings


# ---- Sinh file C ----

HEADER = "/* Thiết kế: docs/design/display/display.md */\n"
GENERATED = "/* Sinh bằng tools/profile_gen.py từ profiles/profiles.json; không sửa tay. */\n"


def hex_list(values):
    return ", ".join(f"0x{value:02X}" for value in values)


def c_profile(profile):
    tables = ",\n".join(f"            {{{hex_list(table)}}}" for table in profile["tables"])
    source = "INK_HAL_DISPLAY_SOURCE_HAND" if profile["source"] == SOURCE_HAND else str(profile["source"])
    return (
        "    {\n"
        f"        .id     = {profile['id']},\n"
        f"        .name   = \"{profile['name']}\",\n"
        f"        .source = {source},\n"
        f"        .scans  = {profile['scans']},\n"
        f"        .pll    = 0x{profile['pll']:02X},\n"
        f"        .power  = {{{hex_list(profile['power'])}}},\n"
        f"        .vcom   = 0x{profile['vcom']:02X},\n"
        f"        .eopt   = 0x{profile['eopt']:02X},\n"
        f"        .xon    = {{{hex_list(profile['xon'])}}},\n"
        f"        .drf_ms = {profile['drf_ms']},\n"
        "        .tables =\n"
        "            {\n"
        f"{tables},\n"
        "            },\n"
        "    },\n"
    )


def c_source(profiles, purposes):
    lines = [HEADER, "\n", GENERATED, "#include \"ink_drv_uc8253_profiles.h\"\n", "\n"]
    lines.append("const ink_drv_uc8253_profile_t ink_drv_uc8253_profiles[INK_DRV_UC8253_PROFILE_COUNT] = {\n")
    lines.extend(c_profile(profile) for profile in profiles)
    lines.append("};\n\n")
    for purpose in purposes:
        bands = ", ".join(f"{{.max_celsius = {celsius}, .profile_id = {profile_id}}}" for celsius, profile_id in purpose["bands"])
        lines.append(f"static const ink_hal_display_band_t s_{purpose['name']}_bands[] = {{{bands}}};\n")
    lines.append("\n")
    lines.append("const ink_hal_display_purpose_default_t ink_drv_uc8253_purpose_defaults[INK_HAL_DISPLAY_PURPOSE_COUNT] = {\n")
    for purpose in purposes:
        enum = "INK_HAL_DISPLAY_PURPOSE_" + purpose["name"].upper()
        flags = (f".partial_mode = {str(purpose['partial_mode']).lower()}, .pt_scan = {str(purpose['pt_scan']).lower()}, "
                 f".drive_border = {str(purpose['drive_border']).lower()}")
        lines.append(f"    [{enum}] = {{.bands = s_{purpose['name']}_bands, .band_count = {len(purpose['bands'])}, "
                     f".config = {{{flags}}}}},\n")
    lines.append("};\n")
    return "".join(lines)


def h_source(profiles):
    return (
        HEADER + "#pragma once\n\n" + GENERATED +
        "#include \"ink_drv_uc8253.h\"\n\n"
        f"#define INK_DRV_UC8253_PROFILE_COUNT {len(profiles)}\n\n"
        "extern const ink_drv_uc8253_profile_t          ink_drv_uc8253_profiles[INK_DRV_UC8253_PROFILE_COUNT];\n"
        "extern const ink_hal_display_purpose_default_t ink_drv_uc8253_purpose_defaults[INK_HAL_DISPLAY_PURPOSE_COUNT];\n"
    )


def clang_format(text, filename):
    exe = shutil.which("clang-format")
    if exe is None:
        return text
    result = subprocess.run([exe, f"--style=file:{os.path.join(ROOT, '.clang-format')}", f"--assume-filename={filename}"],
                            input=text.encode(), capture_output=True, check=True)
    return result.stdout.decode()


# ---- Gói .inkw ----


def crc16(data, crc=0xFFFF):
    """CRC-16/CCITT-FALSE, như khung gói của bench."""
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else crc << 1
            crc &= 0xFFFF
    return crc


def blob(profile):
    name = profile["name"].encode().ljust(NAME_BYTES, b"\0")
    payload = struct.pack("<H", profile["id"]) + name + bytes([profile["source"], profile["scans"], profile["pll"]])
    payload += bytes(profile["power"]) + bytes([profile["vcom"], profile["eopt"]]) + bytes(profile["xon"])
    payload += struct.pack("<H", profile["drf_ms"])
    payload += b"".join(bytes(table) for table in profile["tables"])
    body = bytes([BLOB_VERSION, 0]) + struct.pack("<H", len(payload)) + payload
    return BLOB_MAGIC + body + struct.pack("<H", crc16(body))


# ---- Chạy ----


def write_if_changed(path, text, check_only):
    old = open(path, encoding="utf-8").read() if os.path.exists(path) else None
    if old == text:
        return False
    if check_only:
        print(f"LỖI: {os.path.relpath(path, ROOT)} khác nguồn; chạy python tools/profile_gen.py")
        return True
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write(text)
    print(f"đã ghi {os.path.relpath(path, ROOT)}")
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--blob", metavar="TÊN")
    parser.add_argument("-o", "--output")
    parser.add_argument("--source", default=SOURCE_PATH, help="file profiles.json khác (thử profile mới cho bench)")
    args = parser.parse_args()

    with open(args.source, encoding="utf-8") as file:
        source = json.load(file)
    try:
        profiles, purposes, warnings = build_all(source)
    except ProfileError as error:
        print(f"LỖI: {error}")
        return 1
    for warning in warnings:
        print(f"cảnh báo: {warning}")

    if args.list:
        for profile in profiles:
            origin = "vẽ tay" if profile["source"] == SOURCE_HAND else f"TR{profile['source']}"
            print(f"{profile['id']:5}  {profile['name']:12} {origin:7} {profile['draw_ms']:5} ms  PLL {pll_hz(profile['pll'])} Hz")
        return 0
    if args.blob:
        matches = [profile for profile in profiles if profile["name"] == args.blob]
        if not matches:
            print(f"LỖI: không có profile {args.blob}")
            return 1
        output = args.output or f"{args.blob}.inkw"
        with open(output, "wb") as file:
            file.write(blob(matches[0]))
        print(f"đã ghi {output} ({os.path.getsize(output)} byte)")
        return 0

    c_text = clang_format(c_source(profiles, purposes), C_PATH)
    h_text = clang_format(h_source(profiles), H_PATH)
    failed = write_if_changed(C_PATH, c_text, args.check)
    failed = write_if_changed(H_PATH, h_text, args.check) or failed
    if not failed:
        print(f"OK: {len(profiles)} profile, {len(purposes)} mục đích")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
