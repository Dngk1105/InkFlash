#!/usr/bin/env python3
"""
InkFlash Vision Test - E-ink Display Quality Analyzer.

Performs quantitative optical inspection of captured e-ink frames:
  1. Focus Sharpness: Detects whether camera is in-focus or blurry (Laplacian variance).
  2. Contrast Ratio: Ratio between white paper background and black ink (L_white / L_black).
  3. Illumination Uniformity: Measures gradient/glare across the active display area.
  4. Ghosting Debt %: Measures residual pigment retention in expected white regions.
  5. Flat-field Correction: Algorithmic background equalization.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, Any, Tuple

import cv2
import numpy as np


def compute_sharpness(gray: np.ndarray) -> float:
    """Computes focus sharpness using Laplacian variance."""
    laplacian = cv2.Laplacian(gray, cv2.CV_64F)
    return float(laplacian.var())


def apply_flat_field_correction(img: np.ndarray) -> np.ndarray:
    """
    Equalizes non-uniform illumination across the display using
    large-kernel background estimation and division normalization.
    """
    if len(img.shape) == 3:
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    else:
        gray = img.copy()

    h, w = gray.shape[:2]
    # Filter size ~ 1/10th of diagonal (ensure odd integer)
    ksize = int(max(h, w) * 0.12) | 1
    ksize = max(31, min(ksize, 151))

    # Illumination profile
    illumination = cv2.GaussianBlur(gray, (ksize, ksize), 0)
    mean_val = float(np.mean(illumination))

    # Divide normalized
    norm = (gray.astype(np.float32) / (illumination.astype(np.float32) + 1e-4)) * mean_val
    norm = np.clip(norm, 0, 255).astype(np.uint8)

    if len(img.shape) == 3:
        # Transfer illumination gain to color image
        gain = norm.astype(np.float32) / (gray.astype(np.float32) + 1e-4)
        gain = cv2.cvtColor(gain, cv2.COLOR_GRAY2BGR)
        result = np.clip(img.astype(np.float32) * gain, 0, 255).astype(np.uint8)
        return result
    return norm


def analyze_eink_quality(img_bgr: np.ndarray) -> Dict[str, Any]:
    gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)
    h, w = gray.shape

    sharpness = compute_sharpness(gray)
    focus_status = "SHARP" if sharpness > 120.0 else ("FAIR" if sharpness > 60.0 else "BLURRY")

    # Percentiles for contrast
    p5 = float(np.percentile(gray, 5))    # Black level estimate
    p95 = float(np.percentile(gray, 95))  # White level estimate
    cr = round(p95 / max(p5, 1.0), 2)

    # Uniformity of white background (pixels above median)
    median_val = float(np.median(gray))
    white_pixels = gray[gray >= median_val]
    if len(white_pixels) > 0:
        mean_w = float(np.mean(white_pixels))
        std_w = float(np.std(white_pixels))
        # Uniformity: 1.0 - (std / mean)
        uniformity = round(max(0.0, 1.0 - (std_w / max(mean_w, 1.0))), 3)
    else:
        uniformity = 0.0

    # Ghosting score %: standard deviation in intermediate grays
    # E-ink 2-color ideally has bimodal distribution (low valley between black and white)
    valley_mask = (gray > p5 + 20) & (gray < p95 - 20)
    ghosting_pct = round(float(np.sum(valley_mask)) / float(h * w) * 100.0, 2)

    verdict = "PASS" if cr >= 6.5 and focus_status != "BLURRY" else "WARN"

    return {
        "status": "ok",
        "resolution": f"{w}x{h}",
        "sharpness_score": round(sharpness, 1),
        "focus_status": focus_status,
        "contrast_ratio": cr,
        "black_level": round(p5, 1),
        "white_level": round(p95, 1),
        "uniformity_score": uniformity,
        "ghosting_debt_percent": ghosting_pct,
        "verdict": verdict,
        "recommendation": (
            "Display quality is optimal for testing." if verdict == "PASS"
            else "Check lighting reflection or tap iPhone screen to lock AE/AF focus."
        )
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze optical quality of e-ink screen captures.")
    parser.add_argument("-i", "--input", required=True, help="Path to e-ink screen image")
    parser.add_argument("-o", "--output-corrected", help="Optional path to save flat-field corrected image")
    parser.add_argument("--json", action="store_true", help="Output machine-readable JSON for AI agents")
    args = parser.parse_args()

    img_path = Path(args.input)
    if not img_path.exists():
        err = f"File not found: {img_path}"
        if args.json:
            print(json.dumps({"status": "error", "message": err}))
        else:
            print(f"[ERROR] {err}", file=sys.stderr)
        return 1

    img = cv2.imread(str(img_path))
    if img is None:
        err = f"Cannot decode image: {img_path}"
        if args.json:
            print(json.dumps({"status": "error", "message": err}))
        else:
            print(f"[ERROR] {err}", file=sys.stderr)
        return 1

    metrics = analyze_eink_quality(img)

    if args.output_corrected:
        corrected = apply_flat_field_correction(img)
        out_p = Path(args.output_corrected)
        out_p.parent.mkdir(parents=True, exist_ok=True)
        cv2.imwrite(str(out_p), corrected)
        metrics["corrected_image"] = str(out_p.resolve()).replace("\\", "/")

    if args.json:
        print(json.dumps(metrics, indent=2))
    else:
        print(f"\n--- InkFlash Optical Quality Report ---")
        print(f"Resolution:       {metrics['resolution']}")
        print(f"Focus Sharpness:  {metrics['sharpness_score']} ({metrics['focus_status']})")
        print(f"Contrast Ratio:   {metrics['contrast_ratio']}:1 (White: {metrics['white_level']} / Black: {metrics['black_level']})")
        print(f"Light Uniformity: {metrics['uniformity_score']} / 1.000")
        print(f"Ghosting Debt:    {metrics['ghosting_debt_percent']}%")
        print(f"Verdict:          [{metrics['verdict']}] - {metrics['recommendation']}\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
