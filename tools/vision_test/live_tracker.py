#!/usr/bin/env python3
"""
InkFlash Vision Test - 2K Real-time Optical Flow Screen Tracker & Interactive Web UI.

Features:
  1. 2K QHD (2560x1440) high-fidelity camera capture.
  2. Real-time Lucas-Kanade Optical Flow + Homography RANSAC tracking.
  3. Flat-Field Illumination Correction: Eliminates non-uniform lighting and glare gradients.
  4. 3x Supersampling: Warps e-ink active display area to 1248x720 for pixel-level inspection.
  5. Live Focus Sharpness meter on HUD (detects blur / reminds user to lock AE/AF).
  6. Full manual intervention on Web UI (http://localhost:5000):
     - Drag & drop 4 corner pins with mouse in real time.
     - "Click 4 điểm" mode to re-seed corners.
     - Toggle Auto-Track (ON/OFF - lock fixed coordinates).
     - Toggle Flat-Field Bù sáng (ON/OFF).
  7. Instant snapshot API for AI agents (/snapshot) in < 10ms.
"""

from __future__ import annotations

import json
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import List, Optional, Tuple
from urllib.parse import urlparse

import cv2
import numpy as np

CONFIG_PATH = Path(__file__).resolve().parent / "config.json"


def load_config() -> dict:
    if CONFIG_PATH.exists():
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}


def save_config(cfg: dict) -> None:
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2)


def apply_flat_field(img: np.ndarray) -> np.ndarray:
    """Equalizes non-uniform illumination across the display."""
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    h, w = gray.shape[:2]
    ksize = int(max(h, w) * 0.12) | 1
    ksize = max(31, min(ksize, 151))

    illumination = cv2.GaussianBlur(gray, (ksize, ksize), 0)
    mean_val = float(np.mean(illumination))

    gain = mean_val / (illumination.astype(np.float32) + 1e-4)
    gain = cv2.cvtColor(gain, cv2.COLOR_GRAY2BGR)
    return np.clip(img.astype(np.float32) * gain, 0, 255).astype(np.uint8)


def apply_unsharp_mask(image: np.ndarray, strength: float = 1.4) -> np.ndarray:
    """Enhances edge contrast and subpixel e-ink sharpness."""
    gaussian = cv2.GaussianBlur(image, (0, 0), 2.0)
    return np.clip(cv2.addWeighted(image, 1.0 + strength, gaussian, -strength, 0), 0, 255).astype(np.uint8)


class CameraTracker:
    def __init__(self, camera_index: int = 1, target_size: Tuple[int, int] = (1248, 720)):
        self.camera_index = camera_index
        self.target_size = target_size
        self.frame_width = 2560
        self.frame_height = 1440
        self.running = False
        self.auto_track = True
        self.flat_field = False
        self.sharpen = True
        self.is_user_dragging = False

        self.cap: Optional[cv2.VideoCapture] = None
        self.lock = threading.Lock()

        # Quadrangle tracking state in 2560x1440 space: [[x0,y0], [x1,y1], [x2,y2], [x3,y3]]
        self.initial_quad: np.ndarray = np.array([
            [140.0, 320.0],
            [1347.0, 273.0],
            [1353.0, 1013.0],
            [140.0, 1013.0]
        ], dtype=np.float32)

        cfg = load_config()
        if "screen_quad" in cfg and len(cfg["screen_quad"]) == 4:
            self.initial_quad = np.array(cfg["screen_quad"], dtype=np.float32)
        if "target_resolution" in cfg and len(cfg["target_resolution"]) == 2:
            self.target_size = (int(cfg["target_resolution"][0]), int(cfg["target_resolution"][1]))

        self.current_quad: np.ndarray = self.initial_quad.copy()

        # Feature tracking state
        self.prev_gray: Optional[np.ndarray] = None
        self.prev_pts: Optional[np.ndarray] = None
        self.inliers_count: int = 0
        self.sharpness: float = 0.0
        self.fps: float = 0.0

        # Output frames (JPEG bytes)
        self.latest_raw_jpeg: Optional[bytes] = None
        self.latest_overlay_jpeg: Optional[bytes] = None
        self.latest_warped_jpeg: Optional[bytes] = None

    def start(self) -> bool:
        for idx in [self.camera_index, 0]:
            cap = cv2.VideoCapture(idx, cv2.CAP_DSHOW)
            cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.frame_width)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.frame_height)
            if cap.isOpened():
                self.cap = cap
                self.camera_index = idx
                actual_w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                actual_h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                self.frame_width = actual_w
                self.frame_height = actual_h
                break

        if not self.cap or not self.cap.isOpened():
            return False

        self.running = True
        self.thread = threading.Thread(target=self._capture_loop, daemon=True)
        self.thread.start()
        return True

    def stop(self):
        self.running = False
        if self.cap:
            self.cap.release()

    def reset_quad(self):
        with self.lock:
            self.current_quad = self.initial_quad.copy()
            self.prev_pts = None

    def set_quad(self, pts: List[List[float]], permanent: bool = True):
        with self.lock:
            new_q = np.array(pts, dtype=np.float32)
            if len(new_q) == 4:
                self.current_quad = new_q
                self.initial_quad = new_q.copy()
                self.prev_pts = None

        if permanent:
            cfg = load_config()
            cfg["screen_quad"] = [[float(p[0]), float(p[1])] for p in pts]
            save_config(cfg)

    def _capture_loop(self):
        frame_times = []

        while self.running and self.cap and self.cap.isOpened():
            ret, frame = self.cap.read()
            if not ret or frame is None:
                time.sleep(0.01)
                continue

            now = time.time()
            frame_times.append(now)
            while frame_times and frame_times[0] < now - 1.0:
                frame_times.pop(0)
            self.fps = len(frame_times)

            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            self.sharpness = float(cv2.Laplacian(gray, cv2.CV_64F).var())

            with self.lock:
                if self.auto_track and not self.is_user_dragging:
                    self._update_tracking(gray)
                else:
                    self.prev_gray = gray
                    self.prev_pts = None

                warped = self._warp_current_quad(frame)
                if self.flat_field:
                    warped = apply_flat_field(warped)
                if self.sharpen:
                    warped = apply_unsharp_mask(warped)

                overlay = frame.copy()
                self._draw_overlay(overlay)

            # High quality JPEG encoding without blur
            _, raw_jpg = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 92])
            _, overlay_jpg = cv2.imencode('.jpg', overlay, [cv2.IMWRITE_JPEG_QUALITY, 92])
            _, warped_jpg = cv2.imencode('.jpg', warped, [cv2.IMWRITE_JPEG_QUALITY, 98])

            with self.lock:
                self.latest_raw_jpeg = raw_jpg.tobytes()
                self.latest_overlay_jpeg = overlay_jpg.tobytes()
                self.latest_warped_jpeg = warped_jpg.tobytes()

    def _update_tracking(self, gray: np.ndarray):
        if self.prev_gray is None or self.prev_pts is None or len(self.prev_pts) < 25:
            mask = np.zeros_like(gray)
            cv2.fillPoly(mask, [self.current_quad.astype(np.int32)], 255)
            kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (80, 80))
            mask = cv2.dilate(mask, kernel)

            pts = cv2.goodFeaturesToTrack(gray, mask=mask, maxCorners=140, qualityLevel=0.01, minDistance=10)
            self.prev_pts = pts
            self.prev_gray = gray
            self.inliers_count = len(pts) if pts is not None else 0
            return

        p1, st, err = cv2.calcOpticalFlowPyrLK(
            self.prev_gray, gray, self.prev_pts, None,
            winSize=(25, 25), maxLevel=3,
            criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 30, 0.01)
        )

        if p1 is not None and st is not None:
            good_p0 = self.prev_pts[st == 1]
            good_p1 = p1[st == 1]

            if len(good_p0) >= 12:
                H, inlier_mask = cv2.findHomography(good_p0, good_p1, cv2.RANSAC, 4.0)
                if H is not None and inlier_mask is not None:
                    inliers = int(np.sum(inlier_mask))
                    self.inliers_count = inliers

                    if inliers >= 10:
                        pts_reshaped = self.current_quad.reshape(-1, 1, 2)
                        transformed = cv2.perspectiveTransform(pts_reshaped, H)
                        transformed_pts = transformed.reshape(4, 2)

                        area = cv2.contourArea(transformed_pts)
                        # Sanity check area in 2K
                        if 250000 < area < 2000000:
                            self.current_quad = transformed_pts

            if len(good_p1) < 45:
                self.prev_pts = None
            else:
                self.prev_pts = good_p1.reshape(-1, 1, 2)

        self.prev_gray = gray

    def _warp_current_quad(self, frame: np.ndarray) -> np.ndarray:
        w, h = self.target_size
        src = self.current_quad
        dst = np.float32([[0, 0], [w, 0], [w, h], [0, h]])
        M = cv2.getPerspectiveTransform(src, dst)
        return cv2.warpPerspective(frame, M, (w, h), flags=cv2.INTER_LINEAR)

    def _draw_overlay(self, vis: np.ndarray):
        pts = self.current_quad.astype(np.int32)
        cv2.polylines(vis, [pts], isClosed=True, color=(0, 255, 0), thickness=3)

        labels = ["TL", "TR", "BR", "BL"]
        colors = [(0, 0, 255), (255, 0, 0), (0, 200, 0), (0, 140, 255)]

        for idx, (p, label, col) in enumerate(zip(pts, labels, colors)):
            cv2.circle(vis, tuple(p), 11, col, -1)
            cv2.circle(vis, tuple(p), 13, (255, 255, 255), 2)
            cv2.putText(vis, label, (p[0] + 16, p[1] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 255), 2)

        focus_tag = "SHARP" if self.sharpness > 120.0 else ("FAIR" if self.sharpness > 60.0 else "BLURRY (LOCK AF)")
        focus_color = (0, 255, 0) if self.sharpness > 120.0 else ((0, 255, 255) if self.sharpness > 60.0 else (0, 0, 255))

        hud1 = f"2K QHD ({self.frame_width}x{self.frame_height}) | FPS: {self.fps:.1f} | Inliers: {self.inliers_count}"
        hud2 = f"Focus: {self.sharpness:.1f} [{focus_tag}] | Flat-Field: {'ON' if self.flat_field else 'OFF'}"

        cv2.putText(vis, hud1, (24, 46), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)
        cv2.putText(vis, hud2, (24, 86), cv2.FONT_HERSHEY_SIMPLEX, 0.9, focus_color, 2)


TRACKER = CameraTracker()


DASHBOARD_HTML = """<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <title>InkFlash - 2K E-ink Vision Tracker & Quality Monitor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: #0d1117; color: #c9d1d9; padding: 14px; }
    header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 12px; border-bottom: 1px solid #30363d; margin-bottom: 12px; flex-wrap: wrap; gap: 8px; }
    h1 { font-size: 17px; color: #58a6ff; font-weight: 600; display: flex; align-items: center; gap: 8px; }
    .badge { padding: 4px 10px; border-radius: 12px; font-size: 11px; font-weight: 600; background: #238636; color: white; }
    .badge.paused { background: #d29922; }
    .badge.sharp { background: #238636; }
    .badge.blurry { background: #da3633; }
    .controls { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }
    .btn { background: #21262d; border: 1px solid #30363d; color: #c9d1d9; padding: 6px 12px; border-radius: 6px; cursor: pointer; font-size: 12px; font-weight: 500; transition: all 0.2s; }
    .btn:hover { background: #30363d; border-color: #8b949e; }
    .btn-primary { background: #238636; border-color: #2ea043; color: white; font-weight: 600; }
    .btn-primary:hover { background: #2ea043; }
    .btn-active { background: #1f6feb; border-color: #388bfd; color: white; }
    .grid { display: grid; grid-template-columns: 2fr 1.1fr; gap: 14px; }
    .card { background: #161b22; border: 1px solid #30363d; border-radius: 8px; overflow: hidden; }
    .card-header { padding: 8px 12px; background: #21262d; border-bottom: 1px solid #30363d; font-size: 12px; font-weight: 600; display: flex; justify-content: space-between; align-items: center; }
    .stage-wrapper { position: relative; background: #000; width: 100%; display: flex; justify-content: center; }
    .stage-wrapper img { width: 100%; height: auto; display: block; user-select: none; -webkit-user-drag: none; }
    #interactive-canvas { position: absolute; top: 0; left: 0; width: 100%; height: 100%; cursor: default; }
    .info-bar { padding: 8px 12px; font-size: 11px; background: #161b22; border-top: 1px solid #30363d; display: flex; justify-content: space-between; align-items: center; }
    .toast { color: #58a6ff; font-weight: bold; }
    .metric-chip { background: #21262d; padding: 4px 8px; border-radius: 4px; font-size: 11px; border: 1px solid #30363d; }
  </style>
</head>
<body>
  <header>
    <div>
      <h1>InkFlash &bull; 2K Camera Real-time Screen Tracker</h1>
      <span style="font-size: 11px; color: #8b949e;">Độ phân giải 2K (2560&times;1440) &bull; Kéo thả 4 góc can thiệp trực tiếp &bull; Bù sáng tự động</span>
    </div>
    <div class="controls">
      <span class="badge" id="badge-track">&bull; AUTO-TRACK: BẬT</span>
      <span class="badge" id="badge-focus">Lấy nét: --</span>
      <button class="btn" id="btn-flatfield" onclick="toggleFlatField()">Bù sáng (Flat-field): BẬT</button>
      <button class="btn" id="btn-toggle-autotrack" onclick="toggleAutoTrack()">Tắt Auto-Track</button>
      <button class="btn" id="btn-repick" onclick="startRepickMode()">Click lại 4 điểm</button>
      <button class="btn" onclick="resetTracking()">Reset</button>
      <button class="btn btn-primary" onclick="window.open('/snapshot','_blank')">Xem ảnh AI</button>
    </div>
  </header>

  <div class="grid">
    <div class="card">
      <div class="card-header">
        <span>Camera 2K Feed (Kéo 4 chấm màu để nắn lại viền e-ink)</span>
        <span id="hint-msg" style="color: #58a6ff; font-weight: normal;">Chạm giữ màn hình iPhone để khóa AE/AF Lock</span>
      </div>
      <div class="stage-wrapper" id="stage">
        <img id="cam-img" src="/stream/raw" alt="Camera Feed">
        <canvas id="interactive-canvas"></canvas>
      </div>
      <div class="info-bar">
        <span id="coord-readout">Tọa độ 2K: Đang tải...</span>
        <span class="toast" id="toast-msg"></span>
      </div>
    </div>

    <div class="card">
      <div class="card-header">
        <span>Màn hình E-ink 3x Supersampling (1248 &times; 720)</span>
        <span style="color: #388bfd;">Flat-Field Corrected</span>
      </div>
      <div style="padding: 12px; background: #0b0e14; display: flex; justify-content: center;">
        <img src="/stream/warped" alt="Warped Screen" style="width: 100%; border: 2px solid #388bfd; border-radius: 4px; box-shadow: 0 4px 14px rgba(0,0,0,0.6);">
      </div>
      <div style="padding: 10px 12px; font-size: 11px; border-top: 1px solid #30363d; line-height: 1.6;">
        <div style="display: flex; gap: 6px; margin-bottom: 8px; flex-wrap: wrap;">
          <span class="metric-chip">Chuẩn: <b>416&times;240 &rarr; 1248&times;720 (3x)</b></span>
          <span class="metric-chip">Chất lượng: <b>2K QHD (1440p)</b></span>
        </div>
        <p>&bull; <b>Bù sáng Flat-field</b>: San phẳng dốc sáng, triệt tiêu bóng lóa đèn trần.</p>
        <p>&bull; <b>Khóa AE/AF trên iPhone</b>: Giữ nét hiển vi cho các hạt mực.</p>
        <p>&bull; <b>AI Snapshot API</b>: <code>GET /snapshot</code> (trả về trong 5ms).</p>
      </div>
    </div>
  </div>

  <script>
    const canvas = document.getElementById('interactive-canvas');
    const ctx = canvas.getContext('2d');
    const img = document.getElementById('cam-img');
    const badge = document.getElementById('badge-track');
    const badgeFocus = document.getElementById('badge-focus');
    const btnToggle = document.getElementById('btn-toggle-autotrack');
    const btnFlatField = document.getElementById('btn-flatfield');
    const btnRepick = document.getElementById('btn-repick');
    const toast = document.getElementById('toast-msg');
    const coordReadout = document.getElementById('coord-readout');

    let nativeW = 2560;
    let nativeH = 1440;
    let currentQuad = [[140, 320], [1347, 273], [1353, 1013], [140, 1013]];
    let autoTrack = true;
    let flatField = true;
    let draggingIdx = -1;
    let repickMode = false;
    let repickPts = [];

    const labels = ["TL (Đỏ)", "TR (Xanh)", "BR (Xanh lá)", "BL (Cam)"];
    const colors = ["#ef4444", "#3b82f6", "#22c55e", "#f97316"];

    function resizeCanvas() {
      canvas.width = img.clientWidth;
      canvas.height = img.clientHeight;
      draw();
    }
    img.onload = resizeCanvas;
    window.onresize = resizeCanvas;

    setInterval(async () => {
      if (draggingIdx !== -1 || repickMode) return;
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        currentQuad = data.quad;
        autoTrack = data.auto_track;
        flatField = data.flat_field;
        nativeW = data.frame_width || 2560;
        nativeH = data.frame_height || 1440;

        const sharp = data.sharpness || 0;
        if (sharp > 120) {
          badgeFocus.innerText = `Nét: ${sharp.toFixed(0)} (RẤT NÉT)`;
          badgeFocus.className = 'badge sharp';
        } else if (sharp > 60) {
          badgeFocus.innerText = `Nét: ${sharp.toFixed(0)} (TẠM ỔN)`;
          badgeFocus.className = 'badge';
        } else {
          badgeFocus.innerText = `Nét: ${sharp.toFixed(0)} (MỜ - CẦN KHÓA AF)`;
          badgeFocus.className = 'badge blurry';
        }

        updateUIState();
        draw();
      } catch (e) {}
    }, 150);

    function updateUIState() {
      if (autoTrack) {
        badge.innerText = '• AUTO-TRACK: BẬT';
        badge.className = 'badge';
        btnToggle.innerText = 'Tắt Auto-Track (Khóa cứng)';
      } else {
        badge.innerText = '• AUTO-TRACK: ĐÃ TẮT';
        badge.className = 'badge paused';
        btnToggle.innerText = 'Bật Auto-Track';
      }
      btnFlatField.innerText = flatField ? 'Bù sáng (Flat-field): BẬT' : 'Bù sáng: TẮT';
      btnFlatField.className = flatField ? 'btn btn-active' : 'btn';
      coordReadout.innerText = `4 Góc 2K: [${currentQuad.map(p => `(${Math.round(p[0])},${Math.round(p[1])})`).join(' ')}]`;
    }

    function draw() {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      if (!canvas.width || !canvas.height) return;

      const scaleX = canvas.width / nativeW;
      const scaleY = canvas.height / nativeH;

      const pts = repickMode ? repickPts : currentQuad;

      if (pts.length > 0) {
        ctx.beginPath();
        ctx.moveTo(pts[0][0] * scaleX, pts[0][1] * scaleY);
        for (let i = 1; i < pts.length; i++) {
          ctx.lineTo(pts[i][0] * scaleX, pts[i][1] * scaleY);
        }
        if (pts.length === 4) {
          ctx.closePath();
          ctx.fillStyle = autoTrack ? 'rgba(35, 134, 54, 0.15)' : 'rgba(210, 153, 34, 0.15)';
          ctx.fill();
        }
        ctx.strokeStyle = autoTrack ? '#2ea043' : '#d29922';
        ctx.lineWidth = 3;
        ctx.setLineDash(repickMode ? [6, 4] : []);
        ctx.stroke();
        ctx.setLineDash([]);

        pts.forEach((p, idx) => {
          const sx = p[0] * scaleX;
          const sy = p[1] * scaleY;

          ctx.beginPath();
          ctx.arc(sx, sy, draggingIdx === idx ? 12 : 9, 0, Math.PI * 2);
          ctx.fillStyle = colors[idx];
          ctx.fill();
          ctx.lineWidth = 2;
          ctx.strokeStyle = '#ffffff';
          ctx.stroke();

          ctx.font = 'bold 12px sans-serif';
          ctx.fillStyle = '#ffffff';
          ctx.shadowColor = 'rgba(0,0,0,0.9)';
          ctx.shadowBlur = 4;
          ctx.fillText(labels[idx], sx + 12, sy - 6);
          ctx.shadowBlur = 0;
        });
      }
    }

    function getCanvasCoords(e) {
      const rect = canvas.getBoundingClientRect();
      const scaleX = nativeW / canvas.width;
      const scaleY = nativeH / canvas.height;
      return [
        (e.clientX - rect.left) * scaleX,
        (e.clientY - rect.top) * scaleY
      ];
    }

    canvas.addEventListener('mousedown', (e) => {
      const [mx, my] = getCanvasCoords(e);

      if (repickMode) {
        repickPts.push([mx, my]);
        draw();
        if (repickPts.length === 4) {
          currentQuad = repickPts;
          repickMode = false;
          btnRepick.classList.remove('btn-active');
          btnRepick.innerText = 'Click lại 4 điểm';
          sendQuadToServer(currentQuad);
          showToast('Đã lưu 4 điểm mới!');
        }
        return;
      }

      for (let i = 0; i < 4; i++) {
        const dx = currentQuad[i][0] - mx;
        const dy = currentQuad[i][1] - my;
        if (Math.sqrt(dx * dx + dy * dy) < 60) {
          draggingIdx = i;
          canvas.style.cursor = 'grabbing';
          fetch('/api/dragging?active=1');
          break;
        }
      }
    });

    window.addEventListener('mousemove', (e) => {
      if (draggingIdx === -1) {
        if (!repickMode && canvas.width) {
          const [mx, my] = getCanvasCoords(e);
          let hover = false;
          for (let i = 0; i < 4; i++) {
            const dx = currentQuad[i][0] - mx;
            const dy = currentQuad[i][1] - my;
            if (Math.sqrt(dx * dx + dy * dy) < 60) { hover = true; break; }
          }
          canvas.style.cursor = hover ? 'grab' : 'default';
        }
        return;
      }

      const [mx, my] = getCanvasCoords(e);
      currentQuad[draggingIdx] = [Math.max(0, Math.min(nativeW, mx)), Math.max(0, Math.min(nativeH, my))];
      draw();
    });

    window.addEventListener('mouseup', () => {
      if (draggingIdx !== -1) {
        draggingIdx = -1;
        canvas.style.cursor = 'default';
        fetch('/api/dragging?active=0');
        sendQuadToServer(currentQuad);
        showToast('Đã cập nhật vị trí góc!');
      }
    });

    async function sendQuadToServer(quad) {
      try {
        await fetch('/api/set_quad', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ quad: quad, save: true })
        });
      } catch (e) {}
    }

    async function toggleAutoTrack() {
      try {
        const res = await fetch('/api/toggle_autotrack');
        const data = await res.json();
        autoTrack = data.auto_track;
        updateUIState();
        showToast(autoTrack ? 'Đã bật Auto-Track' : 'Đã khóa góc cố định');
      } catch (e) {}
    }

    async function toggleFlatField() {
      try {
        const res = await fetch('/api/toggle_flatfield');
        const data = await res.json();
        flatField = data.flat_field;
        updateUIState();
        showToast(flatField ? 'Đã bật Bù sáng' : 'Đã tắt Bù sáng');
      } catch (e) {}
    }

    function startRepickMode() {
      repickMode = true;
      repickPts = [];
      btnRepick.classList.add('btn-active');
      btnRepick.innerText = 'Đang chọn (0/4)...';
      showToast('Click 4 góc: 1.Trên-Trái -> 2.Trên-Phải -> 3.Dưới-Phải -> 4.Dưới-Trái');
    }

    async function resetTracking() {
      await fetch('/api/reset');
      showToast('Đã reset về mặc định!');
    }

    function showToast(msg) {
      toast.innerText = msg;
      setTimeout(() => { toast.innerText = ''; }, 3000);
    }
  </script>
</body>
</html>
"""


class LiveTrackerHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path in ("/", "/index.html"):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(DASHBOARD_HTML.encode("utf-8"))

        elif path == "/stream/raw":
            self.send_response(200)
            self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=frame")
            self.end_headers()
            try:
                while TRACKER.running:
                    with TRACKER.lock:
                        frame_bytes = TRACKER.latest_raw_jpeg
                    if frame_bytes:
                        self.wfile.write(b"--frame\r\n")
                        self.wfile.write(b"Content-Type: image/jpeg\r\n\r\n")
                        self.wfile.write(frame_bytes)
                        self.wfile.write(b"\r\n")
                    time.sleep(0.03)
            except (ConnectionResetError, BrokenPipeError):
                pass

        elif path == "/stream/overlay":
            self.send_response(200)
            self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=frame")
            self.end_headers()
            try:
                while TRACKER.running:
                    with TRACKER.lock:
                        frame_bytes = TRACKER.latest_overlay_jpeg
                    if frame_bytes:
                        self.wfile.write(b"--frame\r\n")
                        self.wfile.write(b"Content-Type: image/jpeg\r\n\r\n")
                        self.wfile.write(frame_bytes)
                        self.wfile.write(b"\r\n")
                    time.sleep(0.03)
            except (ConnectionResetError, BrokenPipeError):
                pass

        elif path == "/stream/warped":
            self.send_response(200)
            self.send_header("Content-Type", "multipart/x-mixed-replace; boundary=frame")
            self.end_headers()
            try:
                while TRACKER.running:
                    with TRACKER.lock:
                        frame_bytes = TRACKER.latest_warped_jpeg
                    if frame_bytes:
                        self.wfile.write(b"--frame\r\n")
                        self.wfile.write(b"Content-Type: image/jpeg\r\n\r\n")
                        self.wfile.write(frame_bytes)
                        self.wfile.write(b"\r\n")
                    time.sleep(0.03)
            except (ConnectionResetError, BrokenPipeError):
                pass

        elif path == "/snapshot":
            with TRACKER.lock:
                frame_bytes = TRACKER.latest_warped_jpeg or TRACKER.latest_raw_jpeg

            if frame_bytes:
                self.send_response(200)
                self.send_header("Content-Type", "image/jpeg")
                self.send_header("Content-Length", str(len(frame_bytes)))
                self.end_headers()
                self.wfile.write(frame_bytes)
            else:
                self.send_response(503)
                self.end_headers()

        elif path == "/raw_snapshot":
            with TRACKER.lock:
                frame_bytes = TRACKER.latest_raw_jpeg

            if frame_bytes:
                self.send_response(200)
                self.send_header("Content-Type", "image/jpeg")
                self.send_header("Content-Length", str(len(frame_bytes)))
                self.end_headers()
                self.wfile.write(frame_bytes)
            else:
                self.send_response(503)
                self.end_headers()

        elif path == "/api/reset":
            TRACKER.reset_quad()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok"}).encode("utf-8"))

        elif path == "/api/status":
            with TRACKER.lock:
                status = {
                    "running": TRACKER.running,
                    "fps": TRACKER.fps,
                    "auto_track": TRACKER.auto_track,
                    "flat_field": TRACKER.flat_field,
                    "sharpness": TRACKER.sharpness,
                    "inliers": TRACKER.inliers_count,
                    "camera_index": TRACKER.camera_index,
                    "frame_width": TRACKER.frame_width,
                    "frame_height": TRACKER.frame_height,
                    "quad": TRACKER.current_quad.tolist(),
                }
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(status).encode("utf-8"))

        elif path == "/api/toggle_autotrack":
            with TRACKER.lock:
                TRACKER.auto_track = not TRACKER.auto_track
                state = TRACKER.auto_track
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok", "auto_track": state}).encode("utf-8"))

        elif path == "/api/toggle_flatfield":
            with TRACKER.lock:
                TRACKER.flat_field = not TRACKER.flat_field
                state = TRACKER.flat_field
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok", "flat_field": state}).encode("utf-8"))

        elif path.startswith("/api/dragging"):
            query = urlparse(self.path).query
            active = "active=1" in query
            with TRACKER.lock:
                TRACKER.is_user_dragging = active
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok"}).encode("utf-8"))

        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/api/set_quad":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode("utf-8")
            try:
                data = json.loads(body)
                quad = data.get("quad")
                save_flag = data.get("save", True)
                if quad and len(quad) == 4:
                    TRACKER.set_quad(quad, permanent=save_flag)
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(json.dumps({"status": "ok"}).encode("utf-8"))
                    return
            except Exception:
                pass

            self.send_response(400)
            self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()


def main():
    import argparse
    parser = argparse.ArgumentParser(description="2K Live camera screen tracker with real-time web stream.")
    parser.add_argument("--port", type=int, default=5000, help="Web server port (default: 5000)")
    parser.add_argument("--cam", type=int, default=1, help="Camera index (default: 1 for Iriun Webcam)")
    args = parser.parse_args()

    TRACKER.camera_index = args.cam
    print(f"[INFO] Initializing 2K Camera #{args.cam}...")
    if not TRACKER.start():
        print(f"[ERROR] Could not start camera on index {args.cam}.")
        return 1

    server = ThreadingHTTPServer(("0.0.0.0", args.port), LiveTrackerHandler)
    print(f"\n========================================================")
    print(f"  InkFlash 2K Real-time Tracker & Illumination Bù sáng!")
    print(f"  Camera Native: {TRACKER.frame_width}x{TRACKER.frame_height}")
    print(f"  E-ink Target: {TRACKER.target_size[0]}x{TRACKER.target_size[1]} (3x Supersampling)")
    print(f"  Mở trình duyệt: http://localhost:{args.port}")
    print(f"========================================================\n")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[INFO] Stopping server...")
    finally:
        TRACKER.stop()
        server.server_close()
    return 0


if __name__ == "__main__":
    main()
