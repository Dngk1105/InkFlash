#!/usr/bin/env python3
"""
InkFlash Vision Test - Real-time Frame Capture Tool.

Designed for AI agents (Claude, Codex, Gemini, Antigravity) and developers
to capture photos of the InkFlash e-ink screen using an iPhone (via Lightning
and Iriun Webcam / Camo) or any connected DirectShow webcam.
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import urllib.request
import numpy as np
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

try:
    from PIL import Image, ImageEnhance
except ImportError:
    Image = None  # type: ignore
    ImageEnhance = None  # type: ignore

CONFIG_PATH = Path(__file__).resolve().parent / "config.json"


def load_config() -> Dict[str, Any]:
    default_config = {
        "preferred_device": "Iriun Webcam",
        "fallback_to_any": True,
        "default_output_dir": "tools/vision_test/captures",
        "default_format": "jpg",
        "resolution": "1920x1080",
        "rotate": 0,
        "enhance_contrast": False,
    }
    if CONFIG_PATH.exists():
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                user_config = json.load(f)
                default_config.update(user_config)
        except Exception:
            pass
    return default_config


def find_ffmpeg() -> Optional[str]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg:
        return ffmpeg

    # Common Windows paths
    candidates = [
        Path(os.environ.get("LOCALAPPDATA", "")) / "Microsoft/WinGet/Packages",
        Path("C:/Program Files/ffmpeg/bin/ffmpeg.exe"),
        Path("C:/ffmpeg/bin/ffmpeg.exe"),
    ]
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate)
        if candidate.is_dir():
            found = list(candidate.glob("**/ffmpeg.exe"))
            if found:
                return str(found[0])
    return None


def get_available_devices(ffmpeg_path: str) -> List[str]:
    cmd = [ffmpeg_path, "-list_devices", "true", "-f", "dshow", "-i", "dummy"]
    proc = subprocess.run(cmd, capture_output=True, text=True, errors="ignore")
    output = proc.stderr or proc.stdout or ""

    devices: List[str] = []
    # Match patterns like: [dshow @ ...] "Device Name" (video)
    # or: [in#0 @ ...] "Device Name" (video)
    pattern = re.compile(r'\"([^\"]+)\"\s+\(video\)')
    for match in pattern.finditer(output):
        name = match.group(1).strip()
        if name and name not in devices:
            devices.append(name)
    return devices


def select_device(
    available: List[str], preferred: str, fallback: bool
) -> Optional[str]:
    if not available:
        return None

    # Check for exact match
    for dev in available:
        if dev.lower() == preferred.lower():
            return dev

    # Check for partial match (e.g. "Iriun" in "Iriun Webcam")
    for dev in available:
        if preferred.lower() in dev.lower():
            return dev

    if fallback:
        # Avoid OBS virtual camera if other cameras are available
        for dev in available:
            if "virtual" not in dev.lower():
                return dev
        return available[0]

    return None


def capture_frame(
    ffmpeg_path: str,
    device_name: str,
    output_path: Path,
    resolution: Optional[str] = None,
) -> bool:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    cmd = [
        ffmpeg_path,
        "-y",
        "-f",
        "dshow",
    ]
    if resolution:
        cmd.extend(["-video_size", resolution])

    cmd.extend([
        "-i",
        f"video={device_name}",
        "-vf",
        "select=gte(n\\,8)",
        "-vframes",
        "1",
        "-update",
        "1",
        str(output_path),
    ])

    proc = subprocess.run(cmd, capture_output=True, text=True, errors="ignore")

    # If resolution was rejected by device, retry without -video_size
    if proc.returncode != 0 and resolution:
        cmd_no_res = [
            ffmpeg_path,
            "-y",
            "-f",
            "dshow",
            "-i",
            f"video={device_name}",
            "-vf",
            "select=gte(n\\,8)",
            "-vframes",
            "1",
            "-update",
            "1",
            str(output_path),
        ]
        proc = subprocess.run(
            cmd_no_res, capture_output=True, text=True, errors="ignore"
        )

    return proc.returncode == 0 and output_path.exists() and output_path.stat().st_size > 0


def fetch_live_tracker_frame(output_path: Path, port: int = 5000) -> bool:
    try:
        url = f"http://127.0.0.1:{port}/snapshot"
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req, timeout=0.3) as response:
            if response.status == 200:
                data = response.read()
                if len(data) > 1000:
                    output_path.parent.mkdir(parents=True, exist_ok=True)
                    with open(output_path, "wb") as f:
                        f.write(data)
                    return True
    except Exception:
        pass
    return False


def compute_perspective_coeffs(
    source_quad: List[Tuple[float, float]],
    target_size: Tuple[int, int]
) -> List[float]:
    w, h = target_size
    target_rect = [(0.0, 0.0), (float(w), 0.0), (float(w), float(h)), (0.0, float(h))]

    matrix = []
    for (x, y), (u, v) in zip(target_rect, source_quad):
        matrix.append([x, y, 1, 0, 0, 0, -u * x, -u * y])
        matrix.append([0, 0, 0, x, y, 1, -v * x, -v * y])

    a = np.matrix(matrix, dtype=float)
    b = np.array(source_quad).reshape(8)
    res = np.linalg.solve(a, b)
    return list(np.array(res).reshape(8))


def post_process_image(
    image_path: Path,
    crop: Optional[Tuple[int, int, int, int]] = None,
    quad: Optional[List[Tuple[float, float]]] = None,
    target_res: Optional[Tuple[int, int]] = None,
    rotate: int = 0,
    enhance: bool = False,
) -> Tuple[int, int]:
    if Image is None:
        return (0, 0)

    try:
        with Image.open(image_path) as img:
            modified = False

            if quad and len(quad) == 4 and target_res:
                coeffs = compute_perspective_coeffs(quad, target_res)
                img = img.transform(
                    target_res,
                    Image.Transform.PERSPECTIVE,
                    coeffs,
                    Image.Resampling.BILINEAR
                )
                modified = True

            if rotate in (90, 180, 270):
                img = img.rotate(-rotate, expand=True)
                modified = True

            if crop and len(crop) == 4:
                x, y, w, h = crop
                img = img.crop((x, y, x + w, y + h))
                modified = True

            if enhance and ImageEnhance is not None:
                enhancer = ImageEnhance.Contrast(img)
                img = enhancer.enhance(1.6)
                modified = True

            width, height = img.size
            if modified:
                img.save(image_path)
            return (width, height)
    except Exception:
        return (0, 0)


def resolve_output_path(
    output_arg: Optional[str], default_dir: str, default_ext: str
) -> Path:
    repo_root = Path(__file__).resolve().parents[2]

    if not output_arg:
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"capture_{timestamp}.{default_ext}"
        base_dir = repo_root / default_dir
        return base_dir / filename

    given_path = Path(output_arg)
    if given_path.is_absolute():
        final_path = given_path
    else:
        # Relative paths are placed under default_dir unless starting with .
        if output_arg.startswith(".") or output_arg.startswith("/") or output_arg.startswith("\\"):
            final_path = (repo_root / given_path).resolve()
        else:
            final_path = (repo_root / default_dir / given_path).resolve()

    # Append extension if missing
    if not final_path.suffix:
        final_path = final_path.with_suffix(f".{default_ext}")

    return final_path


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Capture camera frames for AI real-time testing of InkFlash e-ink displays."
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Output file path (supports nested subdirectories like 'tests/card_01/front.jpg')",
    )
    parser.add_argument(
        "-d",
        "--device",
        help="DirectShow camera name (e.g. 'Iriun Webcam', 'Integrated Camera')",
    )
    parser.add_argument(
        "-r",
        "--resolution",
        help="Capture resolution (e.g. '1920x1080', '1280x720')",
    )
    parser.add_argument(
        "--rotate",
        type=int,
        choices=[0, 90, 180, 270],
        default=0,
        help="Rotate image clockwise by degrees",
    )
    parser.add_argument(
        "--crop",
        help="Crop bounding box as 'x,y,w,h' (e.g. '100,150,800,600')",
    )
    parser.add_argument(
        "--contrast",
        action="store_true",
        help="Enhance contrast for sharper e-ink text/ghosting inspection",
    )
    parser.add_argument(
        "--raw",
        action="store_true",
        help="Skip auto perspective warp and return raw full camera frame",
    )
    parser.add_argument(
        "--list-devices",
        action="store_true",
        help="List available camera devices and exit",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output result in machine-readable JSON format for AI agents",
    )
    args = parser.parse_args()

    config = load_config()

    ffmpeg_path = find_ffmpeg()
    if not ffmpeg_path:
        err_msg = "ffmpeg not found in PATH or standard directories."
        if args.json:
            print(json.dumps({"status": "error", "message": err_msg}))
        else:
            print(f"[ERROR] {err_msg}", file=sys.stderr)
        return 2

    devices = get_available_devices(ffmpeg_path)

    if args.list_devices:
        if args.json:
            print(json.dumps({"status": "ok", "devices": devices}))
        else:
            print("Available video capture devices:")
            for idx, dev in enumerate(devices):
                preferred_mark = " (preferred)" if dev.lower() == config.get("preferred_device", "").lower() else ""
                print(f"  [{idx}] {dev}{preferred_mark}")
        return 0

    target_device = args.device or config.get("preferred_device", "Iriun Webcam")
    selected = select_device(
        devices, target_device, fallback=config.get("fallback_to_any", True)
    )

    if not selected:
        err_msg = (
            f"No suitable video capture device found. "
            f"Available devices: {devices if devices else 'None'}. "
            f"Please connect iPhone via Lightning (Iriun Webcam) or specify --device."
        )
        if args.json:
            print(json.dumps({"status": "error", "message": err_msg, "available_devices": devices}))
        else:
            print(f"[ERROR] {err_msg}", file=sys.stderr)
        return 1

    # Output path
    default_dir = config.get("default_output_dir", "tools/vision_test/captures")
    default_format = config.get("default_format", "jpg")
    output_path = resolve_output_path(args.output, default_dir, default_format)

    resolution = args.resolution or config.get("resolution", "1920x1080")

    tracker_used = False
    if not args.raw:
        tracker_used = fetch_live_tracker_frame(output_path)

    if not tracker_used:
        success = capture_frame(
            ffmpeg_path, selected, output_path, resolution=resolution
        )

        if not success:
            err_msg = f"Failed to capture frame from device '{selected}'."
            if args.json:
                print(json.dumps({"status": "error", "message": err_msg}))
            else:
                print(f"[ERROR] {err_msg}", file=sys.stderr)
            return 2

    # Post processing
    quad_points = None
    target_resolution = None
    if not tracker_used and not args.raw and config.get("warp_perspective") and config.get("screen_quad"):
        try:
            raw_pts = config["screen_quad"]
            if len(raw_pts) == 4:
                quad_points = [(float(p[0]), float(p[1])) for p in raw_pts]
                target_res_cfg = config.get("target_resolution", [416, 240])
                target_resolution = (int(target_res_cfg[0]), int(target_res_cfg[1]))
        except Exception:
            quad_points = None

    crop_tuple = None
    if args.crop:
        try:
            parts = [int(p.strip()) for p in args.crop.split(",")]
            if len(parts) == 4:
                crop_tuple = (parts[0], parts[1], parts[2], parts[3])
        except Exception:
            pass

    rotate_deg = args.rotate if args.rotate != 0 else config.get("rotate", 0)
    enhance_flag = args.contrast or config.get("enhance_contrast", False)

    width, height = post_process_image(
        output_path,
        crop=crop_tuple,
        quad=quad_points,
        target_res=target_resolution,
        rotate=rotate_deg,
        enhance=enhance_flag,
    )

    file_size = output_path.stat().st_size
    repo_root = Path(__file__).resolve().parents[2]
    try:
        relative_path = str(output_path.relative_to(repo_root)).replace("\\", "/")
    except ValueError:
        relative_path = str(output_path).replace("\\", "/")

    abs_path_str = str(output_path.resolve()).replace("\\", "/")
    file_uri = f"file:///{abs_path_str}"

    if args.json:
        result = {
            "status": "success",
            "file": abs_path_str,
            "relative_path": relative_path,
            "file_uri": file_uri,
            "device": selected,
            "width": width,
            "height": height,
            "size_bytes": file_size,
            "timestamp": datetime.datetime.now().isoformat(),
        }
        print(json.dumps(result, indent=2))
    else:
        print(f"[OK] Captured: {relative_path} ({width}x{height}, {file_size // 1024} KB)")
        print(f"Device: {selected}")
        print(f"File URI: {file_uri}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
