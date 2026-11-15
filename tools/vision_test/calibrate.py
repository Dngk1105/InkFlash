#!/usr/bin/env python3
"""
InkFlash Vision Test - 4-Point Perspective Calibration Tool.

Enables the developer to pick 4 corners of the e-ink display (Top-Left, Top-Right,
Bottom-Right, Bottom-Left). Once calibrated, capture.py automatically tracks and
warps the perspective, so AI agents receive perfectly rectified, upright e-ink images.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import List, Tuple

import numpy as np
from PIL import Image

try:
    from PyQt5 import QtCore, QtGui, QtWidgets
except ImportError:
    QtWidgets = None  # type: ignore

CONFIG_PATH = Path(__file__).resolve().parent / "config.json"
CAPTURES_DIR = Path(__file__).resolve().parent / "captures" / "calibration"


def load_config() -> dict:
    if CONFIG_PATH.exists():
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}


def save_config(config: dict) -> None:
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2)


def compute_perspective_coeffs(
    source_quad: List[Tuple[float, float]],
    target_size: Tuple[int, int]
) -> List[float]:
    """
    Computes 8 coefficients for PIL.Image.transform PERSPECTIVE.
    source_quad: [(x0, y0), (x1, y1), (x2, y2), (x3, y3)] (TL, TR, BR, BL)
    target_size: (width, height)
    """
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


def warp_image(
    image_path: Path,
    source_quad: List[Tuple[float, float]],
    target_size: Tuple[int, int] = (416, 240)
) -> Image.Image:
    with Image.open(image_path) as img:
        coeffs = compute_perspective_coeffs(source_quad, target_size)
        return img.transform(
            target_size,
            Image.Transform.PERSPECTIVE,
            coeffs,
            Image.Resampling.BILINEAR
        )


class CalibrationWindow(QtWidgets.QMainWindow):
    def __init__(self, image_path: Path, target_size: Tuple[int, int] = (416, 240)):
        super().__init__()
        self.image_path = image_path
        self.target_size = target_size
        self.points: List[QtCore.QPointF] = []
        self.labels = ["1. Trên-Trái (TL)", "2. Trên-Phải (TR)", "3. Dưới-Phải (BR)", "4. Dưới-Trái (BL)"]

        self.setWindowTitle("InkFlash - Căn chỉnh 4 góc màn hình e-ink")
        self.resize(1200, 800)

        self.orig_pixmap = QtGui.QPixmap(str(image_path))
        self.orig_w = self.orig_pixmap.width()
        self.orig_h = self.orig_pixmap.height()

        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        layout = QtWidgets.QVBoxLayout(central)

        # Instruction label
        self.info_label = QtWidgets.QLabel(
            "<b>Hướng dẫn:</b> Click lần lượt 4 góc màn hình: "
            "<span style='color:red;'>1. Trên-Trái</span> → "
            "<span style='color:blue;'>2. Trên-Phải</span> → "
            "<span style='color:green;'>3. Dưới-Phải</span> → "
            "<span style='color:orange;'>4. Dưới-Trái</span>. "
            "(Bấm 'Reset' để chọn lại | Bấm 'Lưu' khi hoàn thành)"
        )
        self.info_label.setStyleSheet("font-size: 14px; padding: 6px; background: #f0f0f0; border-radius: 4px;")
        layout.addWidget(self.info_label)

        # Image view
        self.image_label = ImageClickLabel(self)
        self.image_label.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        layout.addWidget(self.image_label)

        # Buttons
        btn_layout = QtWidgets.QHBoxLayout()
        self.status_label = QtWidgets.QLabel("Chưa chọn điểm nào (0/4)")
        self.status_label.setStyleSheet("font-weight: bold; font-size: 13px;")
        btn_layout.addWidget(self.status_label)

        btn_layout.addStretch()

        self.btn_reset = QtWidgets.QPushButton("Reset (chọn lại)")
        self.btn_reset.clicked.connect(self.reset_points)
        btn_layout.addWidget(self.btn_reset)

        self.btn_save = QtWidgets.QPushButton("Lưu cấu hình (Save)")
        self.btn_save.setEnabled(False)
        self.btn_save.setStyleSheet("background: #2ea44f; color: white; font-weight: bold; padding: 6px 16px;")
        self.btn_save.clicked.connect(self.save_and_close)
        btn_layout.addWidget(self.btn_save)

        layout.addLayout(btn_layout)

    def reset_points(self):
        self.points.clear()
        self.btn_save.setEnabled(False)
        self.status_label.setText("Chưa chọn điểm nào (0/4)")
        self.image_label.update()

    def add_point(self, pt: QtCore.QPointF):
        if len(self.points) < 4:
            self.points.append(pt)
            idx = len(self.points)
            if idx < 4:
                self.status_label.setText(f"Đã chọn {idx}/4 điểm. Tiếp theo: {self.labels[idx]}")
            else:
                self.status_label.setText("Đã chọn đủ 4 góc! Kiểm tra và bấm 'Lưu cấu hình'.")
                self.btn_save.setEnabled(True)
            self.image_label.update()

    def save_and_close(self):
        if len(self.points) == 4:
            raw_coords = [[round(p.x(), 1), round(p.y(), 1)] for p in self.points]
            cfg = load_config()
            cfg["screen_quad"] = raw_coords
            cfg["warp_perspective"] = True
            cfg["target_resolution"] = list(self.target_size)
            save_config(cfg)

            # Test warp
            warped = warp_image(self.image_path, [(p.x(), p.y()) for p in self.points], self.target_size)
            preview_path = CAPTURES_DIR / "warped_preview.jpg"
            preview_path.parent.mkdir(parents=True, exist_ok=True)
            warped.save(preview_path)

            QtWidgets.QMessageBox.information(
                self, "Thành công",
                f"Đã lưu tọa độ 4 góc vào config.json!\nẢnh thử nghiệm lưu tại:\n{preview_path}"
            )
            self.close()


class ImageClickLabel(QtWidgets.QLabel):
    def __init__(self, parent: CalibrationWindow):
        super().__init__()
        self.main_win = parent
        self.setMouseTracking(True)

    def paintEvent(self, event):
        super().paintEvent(event)
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing)

        # Scale image to fit widget
        pix = self.main_win.orig_pixmap.scaled(
            self.size(), QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation
        )

        x_offset = (self.width() - pix.width()) / 2
        y_offset = (self.height() - pix.height()) / 2
        painter.drawPixmap(int(x_offset), int(y_offset), pix)

        self.scale_x = pix.width() / self.main_win.orig_w
        self.scale_y = pix.height() / self.main_win.orig_h
        self.x_off = x_offset
        self.y_off = y_offset

        # Draw points
        colors = [QtGui.QColor(255, 0, 0), QtGui.QColor(0, 0, 255), QtGui.QColor(0, 200, 0), QtGui.QColor(255, 140, 0)]
        poly = QtGui.QPolygonF()

        for idx, pt in enumerate(self.main_win.points):
            screen_x = pt.x() * self.scale_x + self.x_off
            screen_y = pt.y() * self.scale_y + self.y_off
            poly.append(QtCore.QPointF(screen_x, screen_y))

            painter.setBrush(colors[idx % len(colors)])
            painter.setPen(QtGui.QPen(QtCore.Qt.white, 2))
            painter.drawEllipse(QtCore.QPointF(screen_x, screen_y), 7, 7)

            painter.setPen(QtGui.QPen(QtCore.Qt.yellow, 1))
            painter.setFont(QtGui.QFont("Arial", 11, QtGui.QFont.Bold))
            painter.drawText(int(screen_x) + 10, int(screen_y) - 5, self.main_win.labels[idx])

        # Draw polygon lines
        if len(self.main_win.points) > 1:
            pen = QtGui.QPen(QtGui.QColor(0, 255, 255), 2, QtCore.Qt.DashLine)
            painter.setPen(pen)
            painter.drawPolyline(poly)
            if len(self.main_win.points) == 4:
                painter.drawLine(poly.last(), poly.first())

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            widget_x = event.pos().x() - getattr(self, "x_off", 0)
            widget_y = event.pos().y() - getattr(self, "y_off", 0)

            scale_x = getattr(self, "scale_x", 1.0)
            scale_y = getattr(self, "scale_y", 1.0)

            img_x = widget_x / scale_x
            img_y = widget_y / scale_y

            if 0 <= img_x <= self.main_win.orig_w and 0 <= img_y <= self.main_win.orig_h:
                self.main_win.add_point(QtCore.QPointF(img_x, img_y))


def main() -> int:
    parser = argparse.ArgumentParser(description="Calibrate 4 corners of e-ink display for AI auto-warping.")
    parser.add_argument(
        "--points",
        help="Specify 4 points directly as 'x0,y0 x1,y1 x2,y2 x3,y3' (TL TR BR BL)",
    )
    parser.add_argument(
        "--image",
        default="tools/vision_test/captures/calibration/screen_real.jpg",
        help="Path to snapshot image for calibration",
    )
    parser.add_argument(
        "--size",
        default="416x240",
        help="Target output resolution (e.g. 416x240 for landscape, 240x416 for portrait)",
    )
    args = parser.parse_args()

    image_path = Path(args.image)
    if not image_path.exists():
        # Fallback to capture new frame
        print(f"[INFO] Image '{image_path}' not found. Capturing new frame...")
        capture_script = Path(__file__).resolve().parent / "capture.py"
        import subprocess
        subprocess.run([sys.executable, str(capture_script), "-o", str(image_path), "--json"])

    if not image_path.exists():
        print(f"[ERROR] Cannot acquire image at '{image_path}'")
        return 1

    try:
        w_str, h_str = args.size.lower().split("x")
        target_size = (int(w_str), int(h_str))
    except Exception:
        target_size = (416, 240)

    if args.points:
        raw_parts = args.points.strip().split()
        if len(raw_parts) != 4:
            print("[ERROR] --points must contain exactly 4 pairs: 'x0,y0 x1,y1 x2,y2 x3,y3'")
            return 1
        coords = []
        for pair in raw_parts:
            x_s, y_s = pair.split(",")
            coords.append([float(x_s.strip()), float(y_s.strip())])

        cfg = load_config()
        cfg["screen_quad"] = coords
        cfg["warp_perspective"] = True
        cfg["target_resolution"] = list(target_size)
        save_config(cfg)

        warped = warp_image(image_path, [(p[0], p[1]) for p in coords], target_size)
        preview_path = CAPTURES_DIR / "warped_preview.jpg"
        preview_path.parent.mkdir(parents=True, exist_ok=True)
        warped.save(preview_path)

        print("[OK] Calibrated 4 points successfully!")
        print(f"Points: {coords}")
        print(f"Preview saved to: {preview_path}")
        return 0

    if QtWidgets is None:
        print("[ERROR] PyQt5 not available for GUI calibration. Use --points 'x0,y0 x1,y1 x2,y2 x3,y3'")
        return 1

    app = QtWidgets.QApplication(sys.argv)
    win = CalibrationWindow(image_path, target_size)
    win.show()
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
