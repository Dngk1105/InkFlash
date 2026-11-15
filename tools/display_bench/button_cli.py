# Thiết kế: docs/design/display/bench.md
"""Serial CLI tool to emulate button presses for display bench testing.

Enables automated testing and AI agents (Antigravity, Claude, Codex, Gemini)
to control the ESP32-S3 e-ink device without physical human button interaction.
"""

import argparse
import json
import struct
import sys
import time
from typing import Any, Dict, List, Optional, Tuple

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Error: pyserial is required. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)

# ---- Protocol constants (bench.h) ----

BENCH_SYNC_0 = 0xA5
BENCH_SYNC_1 = 0x5A

# Commands (PC -> Device)
BENCH_HELLO = 0x01
BENCH_BUTTON_EMULATE = 0x0E

# Events (Device -> PC)
BENCH_ACK = 0x80
BENCH_INFO = 0x81
BENCH_DRAWN = 0x82
BENCH_GESTURE = 0x83
BENCH_TEMP = 0x84
BENCH_LOG = 0x85

# Button bitmasks (ink_gesture.h)
BUTTON_A = 1 << 0  # 1
BUTTON_B = 1 << 1  # 2
BUTTON_AB = BUTTON_A | BUTTON_B  # 3

# Button actions (bench.h)
ACTION_RELEASE = 0
ACTION_PRESS = 1
ACTION_CLICK = 2
ACTION_DOUBLE_CLICK = 3
ACTION_HOLD = 4

ACTION_NAMES = {
    ACTION_RELEASE: "release",
    ACTION_PRESS: "press",
    ACTION_CLICK: "click",
    ACTION_DOUBLE_CLICK: "double_click",
    ACTION_HOLD: "hold",
}

GESTURE_TYPES = {
    0: "PRESS",
    1: "DOUBLE_PRESS",
    2: "HOLD",
    3: "HOLD_AB_SHORT",
    4: "HOLD_AB_LONG",
}

GESTURE_MODIFIERS = {
    0: "NONE",
    BUTTON_A: "A",
    BUTTON_B: "B",
}

GESTURE_BUTTONS = {
    BUTTON_A: "A",
    BUTTON_B: "B",
    BUTTON_AB: "A+B",
}


def crc16_ccitt(data: bytes, crc: int = 0xFFFF) -> int:
    """CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF)."""
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def build_packet(cmd_type: int, seq: int, payload: bytes = b"") -> bytes:
    """Build packet: A5 5A | type (1) | seq (1) | length (2, LE) | payload | crc16 (2, LE)."""
    length = len(payload)
    header = bytes([BENCH_SYNC_0, BENCH_SYNC_1, cmd_type, seq, length & 0xFF, (length >> 8) & 0xFF])
    crc = crc16_ccitt(header[2:])  # type, seq, length
    crc = crc16_ccitt(payload, crc)
    return header + payload + struct.pack("<H", crc)


def parse_gesture_payload(payload: bytes) -> Dict[str, Any]:
    """Parse BENCH_GESTURE payload: modifier(1), button(1), type(1), pad(1), time_ms(4)."""
    if len(payload) < 8:
        return {"raw_hex": payload.hex()}
    mod, btn, gtype, _, time_ms = struct.unpack("<BBBB I", payload[:8])
    return {
        "source": "BUTTON",
        "button": GESTURE_BUTTONS.get(btn, str(btn)),
        "type": GESTURE_TYPES.get(gtype, str(gtype)),
        "modifier": GESTURE_MODIFIERS.get(mod, str(mod)),
        "time_ms": time_ms,
    }


def find_esp32_port() -> Optional[str]:
    """Auto-detect Espressif USB-Serial-JTAG port (VID:PID 303A:1001)."""
    ports = list(serial.tools.list_ports.comports())
    # Priority 1: match Espressif VID:PID 303A:1001
    for p in ports:
        if p.vid == 0x303A and p.pid == 0x1001:
            return p.device
    # Priority 2: match "USB Serial" in description
    for p in ports:
        if "USB Serial" in (p.description or "") or "JTAG" in (p.description or ""):
            return p.device
    # Priority 3: first COM port if only 1 exists
    if len(ports) == 1:
        return ports[0].device
    return None


class BenchSerialClient:
    """Synchronous communication client for bench firmware protocol."""

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0):
        self.port_name = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.seq = 0
        self.ser: Optional[serial.Serial] = None
        self.buf = bytearray()

    def connect(self):
        try:
            self.ser = serial.Serial(self.port_name, self.baudrate, timeout=0.05)
            self.buf.clear()
        except serial.SerialException as e:
            raise RuntimeError(f"Cannot open serial port '{self.port_name}': {e}")

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send_packet(self, cmd_type: int, payload: bytes = b"") -> int:
        self.seq = (self.seq + 1) & 0xFF
        packet = build_packet(cmd_type, self.seq, payload)
        self.ser.write(packet)
        self.ser.flush()
        return self.seq

    def read_packet(self, deadline: float) -> Optional[Tuple[int, int, bytes]]:
        """Read a single valid packet before deadline."""
        buf = self.buf
        while time.time() < deadline:
            # Process any full packets already present in buf
            while len(buf) >= 6:
                idx = -1
                for i in range(len(buf) - 1):
                    if buf[i] == BENCH_SYNC_0 and buf[i + 1] == BENCH_SYNC_1:
                        idx = i
                        break
                if idx < 0:
                    if len(buf) > 1 and buf[-1] == BENCH_SYNC_0:
                        del buf[:-1]
                    else:
                        buf.clear()
                    break

                if idx > 0:
                    del buf[:idx]

                if len(buf) < 6:
                    break

                cmd_type = buf[2]
                seq = buf[3]
                length = buf[4] | (buf[5] << 8)
                total_len = 6 + length + 2

                if length > 8192:  # Corrupt length
                    del buf[:2]
                    continue

                if len(buf) < total_len:
                    break  # Wait for remaining bytes

                body = buf[2 : 6 + length]
                expected_crc = buf[6 + length] | (buf[6 + length + 1] << 8)
                actual_crc = crc16_ccitt(body)

                packet_payload = bytes(buf[6 : 6 + length])
                del buf[:total_len]

                if actual_crc == expected_crc:
                    return cmd_type, seq, packet_payload
                # Checksum mismatch, advance past sync
                del buf[:2]

            chunk = self.ser.read(self.ser.in_waiting or 1)
            if chunk:
                buf.extend(chunk)

        return None

    def execute_command(
        self,
        cmd_type: int,
        payload: bytes = b"",
        wait_gesture: bool = False,
        expected_gesture_type: Optional[str] = None,
        wait_drawn: bool = False,
        timeout: Optional[float] = None,
    ) -> Dict[str, Any]:
        """Send command, wait for ACK, and optionally wait for GESTURE or DRAWN events."""
        t_timeout = timeout or self.timeout
        deadline = time.time() + t_timeout
        seq = self.send_packet(cmd_type, payload)

        result: Dict[str, Any] = {
            "status": "pending",
            "seq": seq,
            "ack": None,
            "error_code": None,
            "gesture": None,
            "drawn": None,
            "logs": [],
        }

        ack_received = False
        gesture_received = not wait_gesture
        drawn_received = not wait_drawn

        while time.time() < deadline:
            pkt = self.read_packet(deadline)
            if not pkt:
                break
            ptype, pseq, ppayload = pkt

            if ptype == BENCH_ACK and pseq == seq:
                err = struct.unpack("<i", ppayload[:4])[0] if len(ppayload) >= 4 else 0
                result["ack"] = "OK" if err == 0 else f"ERR_{err}"
                result["error_code"] = err
                ack_received = True

            elif ptype == BENCH_GESTURE:
                g_info = parse_gesture_payload(ppayload)
                result["gesture"] = g_info
                if expected_gesture_type:
                    if g_info.get("type") == expected_gesture_type:
                        gesture_received = True
                else:
                    gesture_received = True

            elif ptype == BENCH_DRAWN:
                result["drawn"] = {"length": len(ppayload), "hex": ppayload[:16].hex()}
                drawn_received = True

            elif ptype == BENCH_LOG:
                try:
                    result["logs"].append(ppayload.decode("utf-8", errors="replace"))
                except Exception:
                    pass

            if ack_received and gesture_received and drawn_received:
                break

        if ack_received and result["error_code"] == 0:
            result["status"] = "success"
        elif not ack_received:
            result["status"] = "timeout"
            result["error"] = f"No ACK received within {t_timeout}s"
        else:
            result["status"] = "error"
            result["error"] = f"Firmware error code: {result['error_code']}"

        return result


def parse_button_arg(btn_str: str) -> int:
    """Parse button string (A, B, AB, A+B) to bitmask."""
    b = btn_str.strip().upper()
    if b in ("A", "1"):
        return BUTTON_A
    elif b in ("B", "2"):
        return BUTTON_B
    elif b in ("AB", "A+B", "3", "BOTH"):
        return BUTTON_AB
    raise ValueError(f"Invalid button '{btn_str}'. Use 'A', 'B', or 'AB'.")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument(
        "-p", "--port", type=str, default=None, help="Serial COM port (default: auto-detect ESP32-S3 USB-Serial-JTAG)"
    )
    common.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    common.add_argument("--json", action="store_true", help="Output machine-readable JSON format for AI agents")
    common.add_argument("--timeout", type=float, default=2.5, help="Command timeout in seconds (default: 2.5s)")

    parser = argparse.ArgumentParser(
        parents=[common],
        description="InkFlash Display Bench Serial Button Emulator CLI (for AI & Automated Testing)"
    )

    subparsers = parser.add_subparsers(dest="command", required=True, help="Button command to execute")

    # click <button> [--duration MS]
    p_click = subparsers.add_parser("click", parents=[common], help="Click (press and release after duration)")
    p_click.add_argument("button", type=str, help="Button: A, B, or AB")
    p_click.add_argument("--duration", type=int, default=50, help="Press duration in ms (default: 50)")
    p_click.add_argument("--wait-gesture", action="store_true", default=True, help="Wait for emitted gesture")
    p_click.add_argument("--wait-drawn", action="store_true", help="Wait for screen DRAWN event")

    # double <button>
    p_double = subparsers.add_parser("double", parents=[common], help="Double click button")
    p_double.add_argument("button", type=str, help="Button: A, B, or AB")
    p_double.add_argument("--wait-gesture", action="store_true", default=True, help="Wait for DOUBLE_PRESS gesture")

    # hold <button> [--duration MS]
    p_hold = subparsers.add_parser("hold", parents=[common], help="Hold button down for duration then release")
    p_hold.add_argument("button", type=str, help="Button: A, B, or AB")
    p_hold.add_argument("--duration", type=int, default=650, help="Hold duration in ms (default: 650)")
    p_hold.add_argument("--wait-gesture", action="store_true", default=True, help="Wait for HOLD gesture")

    # combo [--duration MS]
    p_combo = subparsers.add_parser("combo", parents=[common], help="Press buttons A and B simultaneously")
    p_combo.add_argument("--duration", type=int, default=50, help="Duration in ms (default: 50)")
    p_combo.add_argument("--wait-gesture", action="store_true", default=True, help="Wait for gesture")

    # down <button> / press <button>
    p_down = subparsers.add_parser("down", parents=[common], help="Press button down and keep it pressed")
    p_down.add_argument("button", type=str, help="Button: A, B, or AB")

    p_press = subparsers.add_parser("press", parents=[common], help="Alias for down")
    p_press.add_argument("button", type=str, help="Button: A, B, or AB")

    # up <button> / release <button>
    p_up = subparsers.add_parser("up", parents=[common], help="Release button")
    p_up.add_argument("button", type=str, help="Button: A, B, or AB")

    p_release = subparsers.add_parser("release", parents=[common], help="Alias for up")
    p_release.add_argument("button", type=str, help="Button: A, B, or AB")

    # listen [--seconds S]
    p_listen = subparsers.add_parser("listen", parents=[common], help="Listen for events (GESTURE, DRAWN, LOG)")
    p_listen.add_argument("--seconds", type=float, default=5.0, help="Listen duration (default: 5s)")

    args = parser.parse_args()

    port = args.port or find_esp32_port()
    if not port:
        err_msg = "No ESP32-S3 USB serial device found. Specify with --port COMx."
        if args.json:
            print(json.dumps({"status": "error", "error": err_msg}, ensure_ascii=False))
        else:
            print(f"Error: {err_msg}", file=sys.stderr)
        sys.exit(1)

    client = BenchSerialClient(port, args.baud, args.timeout)
    try:
        client.connect()
    except Exception as e:
        if args.json:
            print(json.dumps({"status": "error", "port": port, "error": str(e)}, ensure_ascii=False))
        else:
            print(f"Error connecting to {port}: {e}", file=sys.stderr)
        sys.exit(1)

    try:
        cmd = args.command
        if cmd == "listen":
            deadline = time.time() + args.seconds
            events = []
            if not args.json:
                print(f"Listening on {port} for {args.seconds}s... (Ctrl+C to stop)")
            while time.time() < deadline:
                pkt = client.read_packet(deadline)
                if not pkt:
                    continue
                ptype, seq, payload = pkt
                if ptype == BENCH_GESTURE:
                    info = parse_gesture_payload(payload)
                    events.append({"type": "GESTURE", "data": info})
                    if not args.json:
                        print(f"[GESTURE] {info['button']} {info['type']} (modifier {info['modifier']}) @ {info['time_ms']}ms")
                elif ptype == BENCH_DRAWN:
                    events.append({"type": "DRAWN", "bytes": len(payload)})
                    if not args.json:
                        print(f"[DRAWN] Screen drawn ({len(payload)} bytes)")
                elif ptype == BENCH_LOG:
                    log_text = payload.decode("utf-8", errors="replace")
                    events.append({"type": "LOG", "text": log_text})
                    if not args.json:
                        print(f"[LOG] {log_text}")

            if args.json:
                print(json.dumps({"status": "success", "port": port, "events": events}, ensure_ascii=False))
            return

        # Determine button mask and action
        action = ACTION_CLICK
        duration = 50
        wait_gesture = getattr(args, "wait_gesture", False)
        wait_drawn = getattr(args, "wait_drawn", False)

        if cmd in ("click", "double", "hold", "down", "press", "up", "release"):
            button_mask = parse_button_arg(args.button)
        elif cmd == "combo":
            button_mask = BUTTON_AB
        else:
            raise ValueError(f"Unknown command {cmd}")

        expected_gesture = None
        if cmd == "click":
            action = ACTION_CLICK
            duration = args.duration
            expected_gesture = "PRESS"
        elif cmd == "double":
            action = ACTION_DOUBLE_CLICK
            duration = 50
            expected_gesture = "DOUBLE_PRESS"
        elif cmd == "hold":
            action = ACTION_HOLD
            duration = args.duration
            expected_gesture = "HOLD"
        elif cmd == "combo":
            action = ACTION_CLICK
            duration = args.duration
        elif cmd in ("down", "press"):
            action = ACTION_PRESS
            duration = 0
            wait_gesture = False
        elif cmd in ("up", "release"):
            action = ACTION_RELEASE
            duration = 0
            wait_gesture = True

        payload = struct.pack("<BBH", button_mask, action, duration)
        # Allocate extra timeout if action is hold or double
        extra_timeout = (duration / 1000.0) + 1.5 if duration > 500 or cmd == "double" else args.timeout
        result = client.execute_command(
            BENCH_BUTTON_EMULATE,
            payload,
            wait_gesture=wait_gesture,
            expected_gesture_type=expected_gesture,
            wait_drawn=wait_drawn,
            timeout=extra_timeout,
        )

        result["port"] = port
        result["button"] = GESTURE_BUTTONS.get(button_mask, str(button_mask))
        result["action"] = ACTION_NAMES.get(action, str(action))
        result["duration_ms"] = duration

        if args.json:
            print(json.dumps(result, ensure_ascii=False, indent=2))
        else:
            if result["status"] == "success":
                g = result.get("gesture")
                g_str = f" -> Gesture: {g['button']} {g['type']}" if g else ""
                print(f"OK: Emulated {result['action']} on button {result['button']}{g_str}")
            else:
                print(f"FAILED: {result.get('error', 'unknown error')}", file=sys.stderr)
                sys.exit(1)

    finally:
        client.close()


if __name__ == "__main__":
    main()
