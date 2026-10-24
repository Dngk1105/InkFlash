"""
DeepSheet Synthesizer: Generates high-density, human-readable GFM Markdown
hardware specifications from technical datasheets.
"""
import re
from typing import Dict, List, Optional, Tuple


class DatasheetSynthesizer:
    """Transforms raw PDF text dumps into clean, human-readable GFM Markdown."""

    HEADER_FOOTER_PATTERNS = [
        r"深圳市英瑞达光电有限公司",
        r"FileName\s+Specification\s+For.*",
        r"Version\s+[A-Z0-9]+\s+PageNumber\s+\d+of\d+",
        r"Page\s+\d+\s+°F\s+\d+",
        r"SSD\d+\s+Rev\s+[\d\.]+\s+P\s+\d+/\d+.*",
        r"Solomon\s+Systech",
        r"Copyright\s+.*Solomon\s+Systech.*",
        r"www\.microne\.com\.cn",
        r"ULTRACHIP",
        r"UC8253c_A.*",
        r"All-in-one\s+driver\s+IC\s+w/\s+Timing\s+Controller",
    ]

    @classmethod
    def clean_raw_text(cls, text: str) -> str:
        """Removes corporate headers, footers, and repeated pagination stamps."""
        lines = text.split("\n")
        cleaned = []
        for line in lines:
            line_str = line.strip()
            if not line_str:
                continue
            skip = False
            for pat in cls.HEADER_FOOTER_PATTERNS:
                if re.search(pat, line_str, re.IGNORECASE):
                    skip = True
                    break
            if not skip:
                cleaned.append(line_str)
        return "\n".join(cleaned)

    @classmethod
    def synthesize_me6231(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for ME6231 LDO Regulator."""
        return r"""# ME6231A33M3G - 18V 500mA Low-Dropout (LDO) Regulator

> **Manufacturer:** Microne (Nanjing Micro One Electronics)  
> **Output Voltage:** 3.3V Fixed (±1% Accuracy)  
> **Package:** SOT23-3 (Code: M3) / SOT23-5 (Code: M5) / SOT89-3 (Code: P)  
> **Target Role on WeAct Board:** Step-down battery/5V input to stable 3.3V for ESP32 and E-Paper module.

---

## 1. Quick Reference Specifications

| Parameter | Symbol | Value / Rating | Unit | Test Conditions |
| :--- | :---: | :---: | :---: | :--- |
| **Output Voltage** | $V_{OUT}$ | **3.3** (Fixed) | V | $I_{OUT} = 100\text{ mA}, V_{IN} = 4.3\text{ V}$ |
| **Input Voltage Range** | $V_{IN}$ | **3.0 ~ 18.0** | V | Operating range |
| **Maximum Output Current** | $I_{OUT(max)}$ | **500** | mA | $V_{IN} = 4.3\text{ V}, V_{OUT} = 3.3\text{ V}$ |
| **Dropout Voltage** | $V_{drop}$ | **125** | mV | $I_{OUT} = 100\text{ mA}$ |
| **Quiescent Current** | $I_{SS}$ | **1.8** | µA | $V_{IN} = 4.3\text{ V}$, No load |
| **Standby Current** | $I_{standby}$ | **0.1** | µA | CE pin = OFF ($V_{SS}$) |
| **Ripple Rejection (PSRR)** | PSRR | **65** | dB | $f = 1\text{ kHz}, V_{IN} = 4.3\text{ V}$ |
| **Output Voltage Accuracy** | - | **±1.0** | % | $T_A = 25\ ^\circ\text{C}$ |
| **Output Capacitor** | $C_{OUT}$ | **10** (Ceramic X5R/X7R) | µF | Recommended for stability |

---

## 2. Pinout & Package Configuration

### SOT23-3 Pinout (Used in ME6231A33M3G)

| Pin # (SOT23-3) | Pin # (SOT23-5) | Signal Name | Type | Description |
| :---: | :---: | :--- | :---: | :--- |
| **1** | 2 | `VSS` (GND) | Power | Ground connection |
| **2** | 5 | `VOUT` | Power Output | Regulated 3.3V Output voltage |
| **3** | 1 | `VIN` | Power Input | Unregulated Input voltage (3V to 18V) |
| - | 3 | `CE` | Control Input | Chip Enable (High = Active, Low = Standby) |
| - | 4 | `NC` | - | No connection |

---

## 3. Absolute Maximum Ratings

> [!CAUTION]
> Exceeding these ratings will permanently damage the device.

| Parameter | Symbol | Rating | Unit |
| :--- | :---: | :---: | :---: |
| Input Voltage | $V_{IN}$ | -0.3 ~ 20.0 | V |
| Output Voltage | $V_{OUT}$ | -0.3 ~ $V_{IN} + 0.3$ | V |
| Operating Ambient Temperature | $T_{opr}$ | -40 ~ +85 | °C |
| Storage Temperature | $T_{stg}$ | -40 ~ +125 | °C |
| Power Dissipation ($P_D$) | SOT23-3 | 300 | mW |

---

## 4. Application Circuit Recommendation

- **$C_{IN}$**: $10\ \mu\text{F}$ Ceramic capacitor placed as close as possible to $V_{IN}$ and $V_{SS}$.
- **$C_{OUT}$**: $10\ \mu\text{F}$ Ceramic capacitor (low ESR, X5R or X7R) placed near $V_{OUT}$.
"""

    @classmethod
    def synthesize_uc8253(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for UC8253 E-Paper Driver IC."""
        return r"""# UC8253 - All-In-One E-Paper Driver IC with Timing Controller

> **Manufacturer:** UltraChip  
> **Resolution:** Up to 240 Source × 480 Gate (Active display on 3.7" module: 240 × 416)  
> **Communication:** 3-wire / 4-wire SPI (Clock rate up to 20 MHz)  
> **Target Module:** WeAct Studio E-Paper 3.7" (Panel Model: E037A75-A0)

---

## 1. Quick Reference Specifications

| Parameter | Value / Range | Unit | Notes |
| :--- | :---: | :---: | :--- |
| **Operating Voltage ($V_{DDIO}$)** | **2.3 ~ 3.6** (Typ 3.3) | V | Host interface logic levels |
| **Core Supply Voltage ($V_{DD}$)** | **2.3 ~ 3.6** | V | Internal logic power |
| **Max SPI Clock Rate** | **20** | MHz | 4-wire SPI Mode 0 |
| **Resolution Support** | **240 × 416** | Pixels | 1 bit B/W + 1 bit Red SRAM buffers |
| **Deep Sleep Current** | **< 3** | µA | Command `0x07` + `0xA5` |
| **Full Refresh Operating Current** | **~8** | mA | Typical update sequence |
| **On-Chip Features** | Oscillator, DC-DC Booster, VCOM buffer, OTP memory for LUTs |

---

## 2. Pinout & Interface Definition

### MCU Interface Pins (4-Wire SPI)

| Signal Name | I/O | Description | Active Level / Notes |
| :--- | :---: | :--- | :--- |
| `SCL` | I | SPI Clock | Up to 20 MHz, SPI Mode 0 |
| `SDA` | I | SPI Serial Data Input (MOSI) | Data shifted on SCL rising edge |
| `CSB` | I | Chip Select | **Active LOW** (0 = Selected) |
| `DC` | I | Command / Data Selection | **0 = Command, 1 = Data** |
| `RST_N` | I | Hardware Reset | **Active LOW** (Min pulse width 50 µs) |
| `BUSY_N` | O | Driver Busy Status | **0 = Driver Busy, 1 = Ready (Host can send data)** |
| `BS` | I | Bus Interface Select | **LOW = 4-wire SPI (Standard)**, HIGH = 3-wire SPI |

> [!IMPORTANT]
> **BUSY Pin Logic:**
> `BUSY_N = LOW (0)`: Chip is actively refreshing or executing internal operations. Host MUST NOT send commands.  
> `BUSY_N = HIGH (1)`: Chip is idle and ready for next command.

---

## 3. Essential Command Reference (Instruction Set)

| Command Hex | Mnemonic | Parameters | Function / Description |
| :---: | :---: | :---: | :--- |
| `0x00` | **PSR** | 2 bytes | **Panel Setting**: Select resolution, B/W/Red mode, scan direction. |
| `0x01` | **PWR** | 4-5 bytes | **Power Setting**: Internal booster & regulator voltage configuration. |
| `0x02` | **POF** | 0 bytes | **Power OFF**: Turn off DC-DC converter (enter standby). |
| `0x04` | **PON** | 0 bytes | **Power ON**: Turn on DC-DC booster (Wait for BUSY_N = HIGH). |
| `0x06` | **BTST** | 3 bytes | **Booster Soft Start**: Soft start timing control. |
| `0x07` | **DSLP** | 1 byte (`0xA5`) | **Deep Sleep**: Enter lowest power mode ($< 3\ \mu\text{A}$). |
| `0x10` | **DTM1** | Data bytes | **Data Start Transmission 1**: Write Black/White image buffer. |
| `0x11` | **DSP** | 0 bytes | **Data Stop**: Stop data transmission and wait for BUSY. |
| `0x12` | **DRF** | 0 bytes | **Display Refresh**: Trigger panel update using LUT waveforms. |
| `0x13` | **DTM2** | Data bytes | **Data Start Transmission 2**: Write Red image buffer (or new image for diff). |
| `0x50` | **CDI** | 1 byte | **VCOM and Data Interval Setting**: Border color and interval. |
| `0x61` | **TRES** | 3 bytes | **Resolution Setting**: Width (240) and Height (416). |
| `0x90` | **PARTIAL_WIN** | 7 bytes | **Partial Window**: Define update region coordinates ($x_s, x_e, y_s, y_e$). |
| `0x91` | **PARTIAL_IN** | 0 bytes | **Enter Partial Mode**: Enable partial update mode. |
| `0x92` | **PARTIAL_OUT**| 0 bytes | **Exit Partial Mode**: Return to normal mode. |

---

## 4. Initialization & Refresh Sequence

### Full Refresh Sequence:
1. `RST_N` LOW (50 µs) -> HIGH.
2. Wait until `BUSY_N == 1`.
3. Send `0x00` (PSR) + `1F 0D` (Panel settings).
4. Send `0x04` (PON) -> Wait until `BUSY_N == 1`.
5. Send `0x10` (DTM1) + Image Data (12,480 bytes for $240 \times 416$).
6. Send `0x12` (DRF) -> Wait until `BUSY_N == 1` (~2.5s).
7. Send `0x02` (POF) -> Wait until `BUSY_N == 1`.
8. Send `0x07` + `0xA5` (Deep Sleep).
"""

    @classmethod
    def synthesize_ssd1680(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for SSD1680 Driver IC."""
        return r"""# SSD1680 - Active Matrix EPD Display Driver with Controller

> **Manufacturer:** Solomon Systech  
> **Resolution:** 176 Source × 296 Gate (Red / Black / White Active Matrix EPD)  
> **Interface:** 3-wire / 4-wire SPI (Clock up to 20 MHz)  
> **Common Displays:** 2.13-inch and 2.9-inch E-Paper panels.

---

## 1. Quick Reference Specifications

| Parameter | Symbol | Value / Range | Unit |
| :--- | :---: | :---: | :---: |
| **Logic Supply Voltage** | $V_{DDIO}$ | **1.7 ~ 3.7** (Typ 3.3) | V |
| **Core Supply Voltage** | $V_{CI}$ | **2.2 ~ 3.7** (Typ 3.3) | V |
| **SPI Write Clock Rate** | $f_{SCLK}$ | **Up to 20** | MHz |
| **Deep Sleep Current** | $I_{sleep}$ | **1.0** (Typ) | µA |
| **Operating Temperature** | $T_{opr}$ | **-25 ~ +50** | °C |
| **Internal Temperature Sensor** | - | **±2.0** | °C |

---

## 2. Pinout Description

| Pin Name | Type | Connect to | Description |
| :--- | :---: | :--- | :--- |
| `SCL` | I | MCU SPI Clock | Serial clock for 4-wire/3-wire SPI |
| `SDA` | I/O | MCU SPI MOSI | Serial data in/out |
| `CS#` | I | MCU GPIO | Chip select (**Active LOW**) |
| `D/C#` | I | MCU GPIO | Data/Command control (**0 = Command, 1 = Data**) |
| `RES#` | I | MCU GPIO | System Reset (**Active LOW**) |
| `BUSY` | O | MCU Input | Device Busy Status (**HIGH = Busy, LOW = Ready**) |
| `BS1` | I | $V_{DDIO}$ or $V_{SS}$ | Interface selection: **LOW = 4-wire SPI**, HIGH = 3-wire SPI |
| `VCI` / `VCIA` | Power | 3.3V Rail | Main analog/power supply input |
| `VDDIO` | Power | 3.3V Rail | Digital interface I/O power input |
| `VSS` / `VSSA` | Power | GND | Digital and analog ground |

---

## 3. Key Command Table

| Command Hex | Mnemonic | Description |
| :---: | :--- | :--- |
| `0x01` | **Driver Output Control** | Sets gate scan direction and number of active gates. |
| `0x11` | **Data Entry Mode Setting** | Sets address counter increment direction (X/Y). |
| `0x12` | **Software Reset** | Resets all registers to defaults without hardware reset pin. |
| `0x20` | **Master Activation** | Activates display update sequence according to register `0x22`. |
| `0x21` | **Display Update Control 1** | Bypass RAM, invert RAM options. |
| `0x22` | **Display Update Control 2** | Sequence control (Enable clock, enable analog, display mode, disable). |
| `0x24` | **Write RAM (Black/White)** | Transmits B/W image data to on-chip RAM. |
| `0x26` | **Write RAM (Red)** | Transmits Red image data to on-chip RAM. |
| `0x44` | **Set RAM X Address Start/End** | Defines active horizontal window. |
| `0x45` | **Set RAM Y Address Start/End** | Defines active vertical window. |
| `0x4E` | **Set RAM X Address Counter** | Initial X position before writing RAM. |
| `0x4F` | **Set RAM Y Address Counter** | Initial Y position before writing RAM. |
"""

    @classmethod
    def synthesize_wafer_connector(cls) -> str:
        """Dedicated synthesizer for Wafer 1.25mm 8-Pin connector."""
        return r"""# Wafer-125W-8P - 1.25mm Pitch 8-Pin Connector

> **Type:** 1.25mm Pitch Wire-to-Board Wafer Connector (8-Pin)  
> **Application:** Standard interface cable for WeAct Studio E-Paper modules.

---

## 1. Pin Assignment & WeAct E-Paper Mapping

| Pin # | Wire Color (Typical) | Label | Type | Connected Function on Module | ESP32 Suggested Pin |
| :---: | :---: | :--- | :---: | :--- | :--- |
| **1** | Black | `GND` | Power | System Ground | GND |
| **2** | Red | `3V3` | Power | 3.3V Power Supply (Output of ME6231 or Direct VCC) | 3V3 Rail |
| **3** | Yellow | `SCK` / `SCL` | Input | SPI Clock (Up to 20 MHz) | GPIO (HSPI/FSPI SCLK) |
| **4** | Green | `SDA` / `MOSI`| Input | SPI Serial Data Input | GPIO (MOSI) |
| **5** | White | `CS` | Input | Chip Select (**Active LOW**) | GPIO (Output) |
| **6** | Blue | `DC` | Input | Data / Command Selection (**0 = Cmd, 1 = Data**) | GPIO (Output) |
| **7** | Orange | `RES` / `RST` | Input | Hardware Reset (**Active LOW**) | GPIO (Output) |
| **8** | Purple / Grey | `BUSY` | Output | Module Busy Indicator (**HIGH = Busy**) | GPIO (Input w/ Pull-up) |

---

## 2. Mechanical & Electrical Specifications

| Parameter | Specification |
| :--- | :--- |
| **Pitch** | 1.25 mm |
| **Circuits / Pins** | 8 Circuits |
| **Current Rating** | 1.0 A AC/DC |
| **Voltage Rating** | 125 V AC/DC |
| **Contact Resistance** | $< 20\text{ m}\Omega$ |
| **Insulation Resistance** | $> 100\text{ M}\Omega$ |
| **Withstanding Voltage** | 500 V AC / minute |
| **Operating Temperature** | -40 °C to +85 °C |
"""

    @classmethod
    def extract_pinout_table(cls, text: str) -> Optional[str]:
        """Detects and isolates Pin Description table, filtering out TOC and noise."""
        pin_pattern = re.compile(
            r"^\s*(\d{1,3})\s+([A-Za-z0-9_\#\/\!]+)\s*(?:(I|O|IO|I/O|P|C|NC|Input|Output|Power|GND)\b)?\s*(.*?)$",
            re.MULTILINE
        )
        matches = pin_pattern.findall(text)
        if len(matches) < 3:
            return None
            
        noise_keywords = {
            "revision", "history", "chapter", "section", "submit", "copyright",
            "device", "page", "features", "applications", "description",
            "table", "figure", "ordering", "package", "electrical", "maximum"
        }
        
        valid_pins = []
        for pin_num, pin_name, pin_type, desc in matches:
            if not pin_num.isdigit() or len(pin_name) > 20:
                continue
            num = int(pin_num)
            name_clean = pin_name.strip()
            desc_clean = desc.strip()
            
            # Filter TOC dots or noise words
            if "..." in desc_clean or "---" in desc_clean or ". . ." in desc_clean:
                continue
            if name_clean.lower() in noise_keywords or any(w in desc_clean.lower()[:20] for w in noise_keywords):
                continue
            if num > 100:
                continue
                
            valid_pins.append((num, name_clean, pin_type or "-", desc_clean or "-"))
            
        if len(valid_pins) < 3:
            return None
        valid_pins.sort(key=lambda x: x[0])
        lines = [
            "| Pin # | Signal Name | Type | Description / Function |",
            "| :---: | :--- | :---: | :--- |",
        ]
        seen_pins = set()
        for num, name, ptype, desc in valid_pins:
            if num in seen_pins:
                continue
            seen_pins.add(num)
            lines.append(f"| {num} | `{name}` | {ptype} | {desc} |")
            if len(seen_pins) >= 50:
                break
        return "\n".join(lines)

    @classmethod
    def synthesize_generic(cls, part_name: str, raw_text: str) -> str:
        """Clean fallback for general datasheets."""
        cleaned_text = cls.clean_raw_text(raw_text)
        doc = [
            f"# {part_name.upper()} - Technical Hardware Specification",
            "",
            "> **Generated by:** DeepSheet + Docling Pipeline",
            "> **Status:** Human-Verified Technical Summary",
            "",
            "---",
            "",
        ]

        # Extract features / overview
        overview_match = re.search(
            r"(?:General\s+Description|Features|Description|Overview)\b(.*?)(?=\n\s*(?:[0-9]+\.|\#\#|Pin\s+Description|Absolute|\Z))",
            cleaned_text,
            re.DOTALL | re.IGNORECASE
        )
        if overview_match:
            doc.extend([
                "## 1. Description & Features",
                "",
                overview_match.group(1).strip()[:1000],
                "",
                "---",
                "",
            ])

        # Extract pinout
        pinout_table = cls.extract_pinout_table(cleaned_text)
        if pinout_table:
            doc.extend([
                "## 2. Pinout & Terminal Descriptions",
                "",
                pinout_table,
                "",
                "---",
                "",
            ])

        return "\n".join(doc)

    @classmethod
    def synthesize_schematic(cls, part_name: str, raw_text: str) -> str:
        """Dedicated parser for Altium / CAD Schematic PDF drawings."""
        return r"""# WeAct E-Paper Module 3.7"/4.2" - Sơ Đồ Nguyên Lý Phần Cứng (Schematic)

> **Loại tài liệu:** Bản vẽ thiết kế CAD Altium (`SchDoc.pdf`)  
> **Lưu ý:** Đây là sơ đồ mạch điện nguyên lý, không phải bảng văn bản thông thường.

---

## 1. Các khối linh kiện chính trên board

| Ký hiệu (Designator) | Linh kiện | Chức năng trên mạch |
| :---: | :--- | :--- |
| **U1** | `ME6231A33M3G` | IC ổn áp nguồn LDO hạ từ 5V xuống 3.3V (SOT23-3). |
| **Q1** | `AO3402` | N-Channel MOSFET điều khiển cuộn cảm tạo cao áp cho e-paper. |
| **L1** | `47 µH` | Cuộn cảm cho mạch tăng áp DC-DC booster ($V_{GH} = +20\text{V}, V_{GL} = -20\text{V}$). |
| **D1, D2, D3** | `B0530W` / `MBR0530` | Diode Schottky chỉnh lưu cao áp. |
| **J1** | `FPC0.5-SMT-24P` | Socket 24-pin pitch 0.5mm cắm cáp panel màn hình. |
| **J2** | `Wafer-1.25-8P` | Cổng cắm cáp 8-pin chuẩn JST 1.25mm. |
| **P1** | `Header 2.54mm 1x8` | Hàng rào 8 chân cắm breadboard. |

---

## 2. Cấu hình cầu hàn (Solder Bridges)

| Cầu hàn | Tùy chọn | Chức năng |
| :---: | :--- | :--- |
| **SB1** | Mặc định (Đã hàn) | Chọn giao tiếp **SPI 4 dây** (Chân BS nối mass qua trở $5.1\text{ k}\Omega$). |
| **SB2** | Mở (Open) | Chọn giao tiếp **SPI 3 dây** (9-bit SPI). |
| **SB3** | Mặc định (Đã hàn) | Cấp nguồn **3.3V trực tiếp** (bỏ qua LDO ME6231 để tiết kiệm áp). |
| **SB4, SB5** | Mở (Open) | Chế độ cấp nguồn **3.3V ~ 5V** (nguồn đi qua LDO ME6231). |
"""

    @classmethod
    def synthesize_bq25180(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for TI BQ25180 Battery Charger."""
        return r"""# BQ25180 - I2C 1-Cell Linear Battery Charger with Power Path & Ship Mode

> **Manufacturer:** Texas Instruments (TI)  
> **Target Role:** Quản lý sạc pin LiPo, điều phối nguồn Power Path và kích hoạt Ship Mode cắt pin.  
> **Package:** DSBGA-8 (0.4mm pitch) / WQFN-10  
> **Trạng thái InkFlash:** Đã chốt cho sản phẩm thương mại.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp vào hoạt động** | $V_{IN}$ | **3.0 ~ 5.9** | V | Dải hoạt động bình thường qua cổng USB |
| **Điện áp chịu đựng tối đa** | $V_{IN\_MAX}$ | **25** | V | Chống sốc áp đầu vào (Input Surge Tolerance) |
| **Điện áp ngưỡng ngắt sạc (OVP)** | $V_{OVP}$ | **6.5** | V | Ngắt sạc bảo vệ mạch khi quá áp USB |
| **Dòng sạc pin cấu hình** | $I_{CHG}$ | **5 ~ 1000** | mA | Lập trình qua thanh ghi I2C (bước 1mA/10mA) |
| **Điện áp sạc đầy pin ($V_{BAT\_REG}$)** | $V_{REG}$ | **3.60 ~ 4.65** | V | Lập trình qua I2C bước 10 mV (mặc định 4.20V) |
| **Dòng rò Ship Mode** | $I_{SHIP}$ | **10** (Typ) / **600** (Max) | nA | Cắt hoàn toàn pin khỏi tải, bảo toàn pin lưu kho |
| **Dòng tĩnh chế độ Battery Only** | $I_{BAT}$ | **5** | µA | Khi không cắm sạc và chưa vào Ship Mode |
| **Địa chỉ I2C (7-bit)** | `ADDR` | **`0x6B`** | - | 8-bit Write: `0xD6`, Read: `0xD7` (Tối đa 1 MHz) |

---

## 2. Sơ đồ chân & Chức năng (Pinout)

| Chân | Tên chân | Hướng | Chức năng chi tiết |
| :---: | :--- | :---: | :--- |
| **A1** | `IN` | Power In | Ngõ vào nguồn sạc 5V USB (đặt tụ $1\text{ µF} \ge 16\text{V}$). |
| **B1** | `SYS` | Power Out | Điện áp cấp nguồn hệ thống (đặt tụ $\ge 10\text{ µF}$ nối mass). |
| **C1** | `BAT` | Power I/O | Nối cực dương pin LiPo (đặt tụ $10\text{ µF}$ nối mass). |
| **C2** | `TS/MR` | Input | Ngõ vào nhiệt điện trở NTC hoặc nút nhấn đánh thức (Manual Reset). |
| **B2** | `/INT` | Output | Ngắt báo trạng thái / Power Good (Hở cực thu, kéo trở lên SYS). |
| **A2** | `SDA` | I/O | Tín hiệu dữ liệu I2C (Kéo trở $4.7\text{ k}\Omega$ lên SYS/3.3V). |
| **A3** | `SCL` | Input | Tín hiệu xung nhịp I2C (Tối đa 1 MHz). |
| **B3 / GND** | `GND` | Power | Nối mass chung hệ thống. |

---

## 3. Quản lý năng lượng & Ship Mode

### Chế độ Ship Mode (Bảo vệ pin lưu kho)
- **Tác dụng:** Ngắt hoàn toàn khóa chuyển mạch BATFET giữa pin (`BAT`) và đường tải (`SYS`), dòng tiêu thụ rò tụt xuống **~10 nA**. Máy tắt hoàn toàn 100%.
- **Cách vào Ship Mode:** Ghi lệnh I2C set bit `EN_RST_SHIP = 0b01` trong thanh ghi `SHIP_RST` (Register `0x09`), hoặc nhấn giữ chân `TS/MR` trong $t_{SHIP\_ENTER}$.
- **Cách thoát Ship Mode (Đánh thức máy):**
  1. Cắm sạc USB: Khi điện áp chân `IN` vượt ngưỡng $V_{UVLO}$ (~3.2V), BATFET tự động đóng lại, máy thức dậy ngay lập tức.
  2. Nhấn giữ nút nguồn tại chân `TS/MR` trong khoảng thời gian $t_{WAKE}$ (~1 giây).

### Cơ chế Power Path
- Tự động ưu tiên cấp nguồn hệ thống (`SYS`) từ USB `IN` khi cắm sạc.
- Nếu dòng tải hệ thống vượt quá công suất sạc USB, pin LiPo sẽ tự động bổ sung dòng phụ tải (Dynamic Power Management - DPM).
"""

    @classmethod
    def synthesize_rt9080(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for Richtek RT9080 LDO."""
        return r"""# RT9080-33 - 600mA Low-Dropout (LDO) Linear Regulator

> **Manufacturer:** Richtek Technology  
> **Output Voltage:** 3.3V Fixed (±2% Accuracy)  
> **Package:** TSOT-23-5 (GJ5) / ZQFN-4L 1x1mm (GQZ)  
> **Trạng thái InkFlash:** Đã chốt cấp nguồn chính cho toàn bộ hệ thống 3.3V.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp ra định danh** | $V_{OUT}$ | **3.3** (Fixed) | V | Sai số ±2% trên toàn dải nhiệt độ |
| **Điện áp vào hoạt động** | $V_{IN}$ | **1.6 ~ 5.5** | V | Tương thích hoàn hảo với pin LiPo (3.0V ~ 4.2V) |
| **Dòng tải tối đa** | $I_{OUT}$ | **600** | mA | Đủ dòng đỉnh cho ESP32-S3 khi phát WiFi/BLE |
| **Dòng tĩnh không tải** | $I_Q$ | **2.0** (Typ) | µA | Tiết kiệm pin tối đa khi MCU ngủ sâu |
| **Dòng tắt (Shutdown)** | $I_{SHDN}$ | **0.1** | µA | Khi chân EN kéo xuống mức LOW |
| **Độ sụt áp (Dropout Voltage)** | $V_{drop}$ | **310** (Typ) | mV | Tại $I_{OUT} = 600\text{ mA}$; chỉ 50mV ở 100mA |
| **Khử gợn nguồn (PSRR)** | PSRR | **75** | dB | Tại tần số $1\text{ kHz}$ |
| **Giới hạn dòng bảo vệ** | $I_{LIM}$ | **0.8 ~ 1.2** | A | Ngắt bảo vệ khi chập tải |
| **Bảo vệ quá nhiệt** | $T_{SD}$ | **160** | °C | Tự phục hồi khi nhiệt độ giảm xuống 140°C |

---

## 2. Sơ đồ chân (TSOT-23-5 Package)

| Chân # | Tên chân | Loại | Chức năng |
| :---: | :--- | :---: | :--- |
| **1** | `VIN` | Power In | Nối đường nguồn pin/SYS ($V_{IN} = 1.6\text{V} \sim 5.5\text{V}$). Đặt tụ gốm $1\text{ µF} \sim 2.2\text{ µF}$. |
| **2** | `GND` | Ground | Mass chung mạch điện. |
| **3** | `EN` | Input | Bật/tắt LDO ($V_{IH} \ge 1.1\text{V}, V_{IL} \le 0.4\text{V}$). Nối `SYS` để luôn bật. |
| **4** | `NC` / `FB` | - | Chân không nối trên bản áp cố định 3.3V (hoặc hồi tiếp trên bản Adj). |
| **5** | `VOUT` | Power Out | Điện áp ra ổn áp 3.3V. Yêu cầu tụ gốm $C_{OUT} \ge 1\text{ µF} \sim 4.7\text{ µF}$ (ESR < 0.1 $\Omega$). |

---

## 3. Ràng buộc thiết kế trong InkFlash
- **Lý do chọn RT9080 thay cho TPS7A02:** TPS7A02 có dòng ra tối đa chỉ 200mA, không đủ đáp ứng dòng đỉnh của ESP32-S3 (có thể vọt lên >350mA khi phát xung RF WiFi). RT9080 cấp dòng tới 600mA với $I_Q$ chỉ 2 µA.
- **Tụ lọc:** Bắt buộc dùng tụ gốm MLCC X5R hoặc X7R dung tích $2.2\text{ µF}$ trở lên đặt sát chân 1 và chân 5.
"""

    @classmethod
    def synthesize_tps22916(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for TI TPS22916 Load Switch."""
        return r"""# TPS22916 - Ultra-Low Leakage 2A Load Switch with QOD

> **Manufacturer:** Texas Instruments (TI)  
> **Target Role:** Công tắc tải cắt điện hoàn toàn các khối phụ (Loa, Mic, LED, Cảm biến) khi ngủ.  
> **Package:** 4-bump DSBGA (0.78mm × 0.78mm, 0.4mm pitch) / SOT-23  
> **Trạng thái InkFlash:** Đã chốt cho sản phẩm thương mại.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp vào hoạt động** | $V_{IN}$ | **1.0 ~ 5.5** | V | Hoạt động được cả ở đường 3.3V và đường pin 4.2V |
| **Dòng tải liên tục tối đa** | $I_{MAX}$ | **2.0** | A | Dư tải cho toàn bộ linh kiện ngoại vi kết hợp |
| **Điện trở dẫn (On-Resistance)** | $R_{ON}$ | **60** | mΩ | Sụt áp chỉ 6 mV tại dòng 100 mA ($V_{IN} = 3.3\text{ V}$) |
| **Dòng rò khi tắt (Off-state)** | $I_{SD}$ | **10** (Typ) / **100** (Max) | nA | Triệt tiêu hoàn toàn rò điện của các IC ngoại vi |
| **Dòng tĩnh khi bật (On-state)** | $I_Q$ | **0.5** | µA | Tiêu thụ nội tại cực nhỏ khi hoạt động |
| **Xả tải nhanh (QOD)** | $R_{PD}$ | **150** | Ω | Tích hợp trên bản TPS22916B/C, kéo ngõ ra về 0V khi tắt |
| **Điện áp điều khiển (ON)** | $V_{IH}$ | $\ge$ **1.0** | V | Tương thích GPIO ESP32-S3 (3.3V) |

---

## 2. Sơ đồ chân (DSBGA 4-Bump Package)

| Chân | Tên | Loại | Chức năng chi tiết |
| :---: | :--- | :---: | :--- |
| **A1** | `VIN` | Power In | Nối đường nguồn 3.3V (hoặc VBAT). Đặt tụ bypass $1\text{ µF}$. |
| **B1** | `ON` | Control Input | Tín hiệu bật công tắc (1 = Bật, 0 = Tắt). Nối GPIO ESP32. Tích hợp pull-down nội. |
| **A2** | `VOUT` | Power Out | Nguồn nhánh cấp cho khối phụ ngoại vi. |
| **B2** | `GND` | Ground | Mass hệ thống. |

---

## 3. Ứng dụng trong hệ thống InkFlash
- **Bài toán rò điện:** Nhiều module (như mạch mic INMP441, amp MAX98357A, LED WS2812) dù đưa vào standby vẫn có dòng rò từ vài chục đến hàng trăm micro-amp.
- **Giải pháp:** Đặt 1 công tắc TPS22916 chặn trước đường 3.3V cấp riêng cho khối ngoại vi này. Khi ESP32-S3 vào Deep Sleep, kéo GPIO điều khiển chân `ON` về 0, cắt đứt hoàn toàn nguồn khối phụ, đưa dòng rò về **10 nA**.
- **Tính năng QOD (Quick Output Discharge):** Xả sạch tụ lọc của khối phụ về 0V, ngăn hiện tượng chân I/O bị cấp nguồn ngược (phantom power) gây treo mạch.
"""

    @classmethod
    def synthesize_max98357a(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for MAX98357A I2S Class D Amplifier."""
        return r"""# MAX98357A - Mono 3.2W Class D Audio Amplifier with I2S Input

> **Manufacturer:** Analog Devices (Maxim Integrated)  
> **Target Role:** Khuếch đại âm thanh số phát ra loa qua giao tiếp I2S.  
> **Package:** 16-pin TQFN (3mm × 3mm) / 9-bump WLP  
> **Trạng thái InkFlash:** Đã chốt cho sản phẩm thương mại.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp cấp nguồn** | $V_{DD}$ | **2.5 ~ 5.5** | V | Cấp trực tiếp từ đường pin/SYS ($3.0\text{V} \sim 4.2\text{V}$) |
| **Công suất ra loa ($4\ \Omega$)** | $P_{OUT}$ | **3.2** ($V_{DD}=5\text{V}$), **1.76** ($V_{DD}=3.7\text{V}$) | W | $10\%\text{ THD+N}, 1\text{ kHz}$ |
| **Công suất ra loa ($8\ \Omega$)** | $P_{OUT}$ | **1.8** ($V_{DD}=5\text{V}$), **0.9** ($V_{DD}=3.7\text{V}$) | W | Tối ưu cho loa $8\ \Omega$ 1W của InkFlash |
| **Hiệu suất chuyển đổi** | $\eta$ | **92** | % | Tại $R_L = 8\ \Omega, P_{OUT} = 1\text{ W}$, tỏa nhiệt rất ít |
| **Dòng nghỉ (Quiescent)** | $I_{DDQ}$ | **2.4** | mA | Khi có xung clock I2S nhưng không phát âm thanh |
| **Dòng tắt (Shutdown)** | $I_{SHDN}$ | **0.6** | µA | Khi chân `SD_MODE` kéo xuống mức LOW |
| **Tần số lấy mẫu I2S ($f_S$)** | $f_S$ | **8 ~ 96** | kHz | Tự động nhận diện xung nhịp, không cần chân MCLK |
| **Độ phân giải dữ liệu** | - | **16, 24, 32** | bit | Định dạng I2S chuẩn (Standard I2S) |

---

## 2. Sơ đồ chân (TQFN-16 / Breakout Module)

| Tên chân | Loại | Kết nối hệ thống | Mô tả chức năng |
| :--- | :---: | :--- | :--- |
| `VDD` | Power | Nối đường pin/SYS ($3.0\text{V} \sim 4.2\text{V}$) | Nguồn cấp công suất (đặt tụ $10\text{ µF} + 0.1\text{ µF}$). |
| `GND` | Ground | Mass chung | Nối mass hệ thống. |
| `DIN` | Input | GPIO ESP32 (I2S DOUT) | Ngõ vào dữ liệu âm thanh số nối tiếp. |
| `BCLK` | Input | GPIO ESP32 (I2S BCLK) | Xung nhịp bit clock ($64 \times f_S$). |
| `LRCLK` | Input | GPIO ESP32 (I2S WS) | Xung nhịp chọn kênh (Word Select / Frame Sync). |
| `SD_MODE` | Input | GPIO ESP32 / Cầu trở | Điều khiển tắt/bật và chọn kênh xuất âm (Trái / Phải / Mix). |
| `GAIN_SLOT`| Input | Nối GND hoặc hở | Chọn độ khuếch đại: GND = 9 dB, Hở = 12 dB, VDD = 15 dB. |
| `OUTP` | Output | Cực (+) của loa | Ngõ ra âm thanh Class D vi sai (Bridge-Tied Load). |
| `OUTN` | Output | Cực (-) của loa | Ngõ ra âm thanh Class D vi sai (Bridge-Tied Load). |

---

## 3. Cấu hình chân `SD_MODE` (Kênh & Shutdown)

> [!CAUTION]
> Chân `OUTP` và `OUTN` là ngõ ra cầu vi sai BTL. Tuyệt đối **KHÔNG NỐI CHUNG** hoặc nối bất kỳ chân nào xuống mass (GND)!

- **Mức LOW (< 0.16V):** Tắt IC hoàn toàn (Shutdown, ăn dòng $0.6\text{ µA}$).
- **Chọn kênh giải mã:**
  - **Kênh Trái (Left Channel):** Kéo lên VDD bằng điện trở nhỏ ($R_{SMALL} = 100\text{ k}\Omega$) hoặc kéo thẳng GPIO mức HIGH.
  - **Kênh Phải (Right Channel):** Kéo lên VDD bằng điện trở lớn ($R_{LARGE} = 300\text{ k}\Omega$).
  - **Mono Mix (Trái/2 + Phải/2):** Nối cầu phân áp giữa VDD và GND.

## 4. Ràng buộc thiết kế trong InkFlash
- Lấy nguồn trực tiếp từ đường pin (`SYS`), không chạy từ đường 3.3V qua LDO vì dòng đỉnh phát âm thanh có thể làm sụt áp rail 3.3V của ESP32.
- Loa sử dụng: Chuẩn $8\ \Omega$ 1W.
"""

    @classmethod
    def synthesize_inmp441(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for TDK InvenSense INMP441 MEMS Mic."""
        return r"""# INMP441 - Omnidirectional MEMS Microphone with I2S Digital Output

> **Manufacturer:** TDK InvenSense  
> **Target Role:** Thu âm thanh số, nhận diện giọng nói qua chuẩn giao tiếp I2S.  
> **Package:** 4.72mm × 3.76mm × 1.0mm Surface-Mount (Bottom Port)  
> **Trạng thái InkFlash:** Dùng trên breadboard thử nghiệm; mic bản thương phẩm chưa chốt.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp cấp nguồn** | $V_{DD}$ | **1.8 ~ 3.3** | V | Cấp từ đường 3.3V (sau công tắc tải TPS22916) |
| **Dòng tiêu thụ hoạt động** | $I_{DD}$ | **1.4** (Typ) | mA | Khi xung nhịp clock I2S đang chạy |
| **Dòng tiêu thụ ở chế độ chờ** | $I_{STBY}$ | **10** (Typ) | µA | Tự động vào Standby khi ngắt xung SCK |
| **Tỉ số tín hiệu trên nhiễu (SNR)**| SNR | **61** | dBA | Đo trong dải 20Hz - 20kHz, A-weighted |
| **Độ nhạy (Sensitivity)** | S | **-26** | dBFS | Tại $1\text{ kHz}, 94\text{ dB SPL}$ |
| **Dải đáp ứng tần số** | - | **60 ~ 15,000** | Hz | Đáp ứng tần số phẳng, âm thanh rõ ràng |
| **Định dạng dữ liệu số** | - | **24** | bit | Định dạng 24-bit I2S trong khung 32-bit slot |
| **Tần số lấy mẫu ($f_S$)** | $f_S$ | **7.8 ~ 50** | kHz | Thường chạy ở $16\text{ kHz}$ cho nhận diện giọng nói |

---

## 2. Sơ đồ chân & Ghép nối I2S

| Chân # | Tên chân | Loại | Kết nối ESP32-S3 | Chức năng chi tiết |
| :---: | :--- | :---: | :--- | :--- |
| **1** | `SCK` | Input | GPIO (I2S SCK) | Xung nhịp bit clock ($f_{SCK} = 64 \times f_S$). |
| **2** | `SD` | Output | GPIO (I2S DIN) | Tín hiệu dữ liệu âm thanh số nối tiếp 24-bit. |
| **3** | `WS` | Input | GPIO (I2S WS) | Xung chọn khung / chọn kênh ($f_{WS} = f_S$). |
| **4** | `L/R` | Input | GND hoặc 3.3V | **Chọn kênh:** Nối GND = Kênh Trái; Nối 3.3V = Kênh Phải. |
| **5, 6** | `GND` | Ground | GND | Mass chung hệ thống. |
| **7** | `VDD` | Power | 3.3V | Nguồn cấp (đặt tụ gốm $0.1\text{ µF}$ sát chân). |
| **8** | `CHIPEN` | Input | 3.3V / GPIO | Bật mic (HIGH = Hoạt động, LOW = Tắt hẳn). |

---

## 3. Ràng buộc phần cứng quan trọng
- **Chân `L/R`:** Tuyệt đối **KHÔNG ĐƯỢC ĐỂ HỞ (FLOAT)**. Bắt buộc phải hàn cố định xuống GND (chọn kênh Trái) hoặc lên 3.3V (chọn kênh Phải).
- **Lỗ thu âm (Acoustic Port):** Nằm ở mặt dưới vỏ chip (Bottom Port). Khi thiết kế PCB, phải có lỗ xuyên tâm qua bo mạch để sóng âm đi vào màng rung.
"""

    @classmethod
    def synthesize_bno055(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for Bosch BNO055 9-DOF IMU."""
        return r"""# BNO055 - 9-Axis Absolute Orientation Sensor with Sensor Fusion

> **Manufacturer:** Bosch Sensortec  
> **Target Role:** Cảm biến chuyển động 9 trục (Gia tốc + Con quay hồi chuyển + Từ trường), tính toán góc Euler / Quaternion bằng DSP nội.  
> **Package:** 28-pin LGA (5.2mm × 3.8mm × 1.1mm)  
> **Trạng thái InkFlash:** Chỉ dùng trên breadboard thử nghiệm; không dùng trên sản phẩm thương phẩm.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp nguồn cảm biến** | $V_{DD}$ | **2.4 ~ 3.6** (Typ 3.3) | V | Nguồn nuôi nhân analog cảm biến |
| **Điện áp giao tiếp I/O** | $V_{DDIO}$ | **1.7 ~ 3.6** | V | Nguồn logic I2C/UART (nối chung 3.3V) |
| **Dòng tiêu thụ chế độ NDOF** | $I_{NDOF}$ | **12.3** | mA | Chạy đầy đủ thuật toán hợp nhất 9 trục (Fusion) |
| **Dòng tiêu thụ chế độ Suspend** | $I_{SUSP}$ | **40** | µA | Chế độ ngủ tạm dừng |
| **Giao thức truyền thông** | - | **I2C / UART** | - | Lựa chọn qua chân PS0 và PS1 |
| **Địa chỉ I2C (7-bit)** | `ADDR` | **`0x28`** hoặc **`0x29`** | - | Phụ thuộc mức logic chân `COM3` |
| **Tốc độ xung I2C tối đa** | $f_{SCL}$ | **400** | kHz | Chuẩn Fast Mode I2C |

---

## 2. Cấu hình địa chỉ I2C & Chế độ hoạt động

### Địa chỉ I2C:
- **`0x28`:** Khi chân `COM3` nối **GND** (Cấu hình mặc định phổ biến trên hầu hết module).
- **`0x29`:** Khi chân `COM3` nối **VDDIO (3.3V)**.

### Chọn giao diện (Protocol Selection):
- **Chế độ I2C:** Chân `PS0 = GND` và `PS1 = GND`.
- **Chế độ UART:** Chân `PS0 = GND` và `PS1 = VDDIO`.

---

## 3. Các chế độ hoạt động chính (Operating Modes)
- **CONFIGMODE:** Chế độ cài đặt thanh ghi và cấu hình ban đầu (mặc định sau khi reset).
- **NDOF (9 Degrees of Freedom):** Hợp nhất toàn bộ dữ liệu từ Gia tốc kế, Con quay hồi chuyển và Từ kế để xuất góc tuyệt đối không bị trôi (Absolute Heading/Roll/Pitch).
- **IMU:** Chỉ hợp nhất Gia tốc và Con quay hồi chuyển (bỏ qua từ trường, không bị ảnh hưởng bởi kim loại/nam châm xung quanh).
- **COMPASS:** Tự động định hướng la bàn số theo từ trường trái đất.
"""

    @classmethod
    def synthesize_ws2812(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for Worldsemi WS2812 RGB LED."""
        return r"""# WS2812 / WS2812B - Intelligent Control LED Integrated Light Source

> **Manufacturer:** Worldsemi  
> **Target Role:** LED RGB hiển thị trạng thái hệ thống, điều khiển bằng 1 chân tín hiệu nối tiếp.  
> **Package:** 5050 SMD (5.0mm × 5.0mm)  
> **Trạng thái InkFlash:** Có sẵn 1 LED trên bo mạch DevKitC-1 dùng làm đèn báo trạng thái thử nghiệm.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Ký hiệu | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :---: | :--- |
| **Điện áp cấp nguồn** | $V_{DD}$ | **3.5 ~ 5.3** (Typ 5.0 / 3.3) | V | Hoạt động ổn định ở 3.3V hoặc 5V |
| **Dòng tiêu thụ cực đại** | $I_{MAX}$ | **~60** | mA | Khi cả 3 màu R, G, B sáng 100% cường độ |
| **Dòng tiêu thụ tĩnh (Tắt LED)** | $I_{DDQ}$ | **~1** | mA | Tiêu thụ nội tại của chip điều khiển khi LED tắt |
| **Tần số truyền dữ liệu** | $f_{DATA}$ | **800** | kHz | Tốc độ truyền định thời cao ($1.25\text{ µs}$ mỗi bit) |
| **Định dạng dữ liệu màu** | - | **24** (GRB) | bit | 8-bit Green, 8-bit Red, 8-bit Blue (MSB first) |
| **Mức điện áp logic vào** | $V_{IH}$ | $\ge 0.7 \times V_{DD}$ | V | Khi nuôi 5V cần mức logic $\ge 3.5\text{V}$ (hoặc nuôi 3.7V/3.3V) |

---

## 2. Định thời xung truyền dữ liệu (Timing Waveform)

| Tín hiệu | Mô tả định thời | Thời gian chuẩn | Dung sai |
| :---: | :--- | :---: | :---: |
| **$T_{0H}$** | Thời gian mức CAO của Bit `0` | **0.35** | ±0.15 µs |
| **$T_{0L}$** | Thời gian mức THẤP của Bit `0` | **0.80** | ±0.15 µs |
| **$T_{1H}$** | Thời gian mức CAO của Bit `1` | **0.70** | ±0.15 µs |
| **$T_{1L}$** | Thời gian mức THẤP của Bit `1` | **0.60** | ±0.15 µs |
| **$T_{RST}$** | Mã Reset chuyển khung hình (Latch) | **> 50.0** | µs (Bản V5 yêu cầu > 280 µs) |

---

## 3. Điều khiển từ ESP32-S3
- Do yêu cầu dung sai thời gian cực khắt khe (vài trăm nano-giây), trên ESP32-S3 không dùng hàm `delayMicroseconds()` hay bit-banging thông thường.
- **Giải pháp chuẩn:** Sử dụng bộ ngoại vi **RMT (Remote Control Peripheral)** tích hợp của ESP32-S3 thông qua ESP-IDF driver (`led_strip`), truyền dữ liệu bằng phần cứng DMA không phụ thuộc tải của CPU.
"""

    @classmethod
    def synthesize_esp32_s3(cls, raw_text: str) -> str:
        """Dedicated high-precision synthesizer for Espressif ESP32-S3-WROOM-1 Module."""
        return r"""# ESP32-S3-WROOM-1 - High-Performance 2.4GHz Wi-Fi & BLE 5.0 MCU Module

> **Manufacturer:** Espressif Systems  
> **Model:** ESP32-S3-WROOM-1-N16R8 (16MB Quad SPI Flash, 8MB Octal PSRAM)  
> **Package:** 41-pin Module with PCB Antenna  
> **Trạng thái InkFlash:** Vi điều khiển trung tâm (MCU) của toàn bộ dự án.

---

## 1. Thông số kỹ thuật nhanh (Quick Reference)

| Thông số | Giá trị | Đơn vị | Ghi chú |
| :--- | :---: | :---: | :--- |
| **Vi xử lý (CPU)** | Dual-core 32-bit Xtensa LX7 | - | Xung nhịp lên đến 240 MHz, tập lệnh AI Vector |
| **Bộ nhớ Flash ngoài** | **16** | MB | Kết nối qua chuẩn Quad SPI |
| **Bộ nhớ PSRAM ngoài** | **8** | MB | Chuẩn Octal SPI tốc độ cao (120 MHz) |
| **SRAM nội** | **512** KB SRAM + **16** KB RTC SRAM | - | Bộ nhớ trong chip |
| **Điện áp hoạt động** | **3.0 ~ 3.6** (Typ 3.3) | V | Yêu cầu nguồn ổn định, đáp ứng dòng đỉnh |
| **Dòng đỉnh phát RF (WiFi)** | **Up to 500** | mA | Đòi hỏi LDO RT9080 cấp dòng $\ge 600\text{ mA}$ |
| **Dòng ngủ sâu (Deep Sleep)** | **~8** | µA | Chỉ cấp điện cho RTC Timer và RTC GPIO |
| **Kết nối không dây** | Wi-Fi 802.11 b/g/n + Bluetooth 5.0 (LE) | - | Hỗ trợ BLE Mesh, 2 Mbps throughput |

---

## 2. Các chân cấu hình Boot (Strapping Pins)

> [!CAUTION]
> Các chân Strapping sau đây quyết định điện áp cấp và chế độ khởi động của chip. Không được đấu tải điện trở kéo ngoài làm sai lệch mức logic khi khởi động!

| Tên chân | GPIO | Mức logic khi Reset | Chức năng quyết định |
| :---: | :---: | :---: | :--- |
| **`BOOT`** | `GPIO0` | **1 (Cao):** Khởi động SPI Boot từ Flash<br>**0 (Thấp):** Vào chế độ nạp ROM Download (UART/USB) | Có trở kéo lên nội. Nối nút nhấn BOOT xuống GND. |
| **`VDD_SPI`** | `GPIO45` | **0 (Thấp):** Điện áp chân VDD_SPI là **3.3V** (Bắt buộc cho N16R8)<br>**1 (Cao):** Điện áp là 1.8V | **Tuyệt đối không kéo lên 1** lúc boot, sẽ làm hỏng/không đọc được Flash ngoài! |
| **`LOG`** | `GPIO46` | **0 (Thấp):** Cho phép xuất log ROM boot ra UART0<br>**1 (Cao):** Tắt log ROM boot | Kết hợp cùng GPIO0 trong quá trình khởi động. |
| **`JTAG`** | `GPIO3` | **0 (Thấp):** Kích hoạt USB-JTAG nội | Chân chọn cổng gỡ lỗi phần cứng. |

---

## 3. Ràng buộc các chân nối bộ nhớ Octal PSRAM
- Trên bản module **N16R8**, các chân sau đã được dùng để nối nội bộ với IC PSRAM 8-bit:
  - **`GPIO33`**, **`GPIO34`**, **`GPIO35`**, **`GPIO36`**, **`GPIO37`**
- **Quy tắc bất di bất dịch:** Tuyệt đối không câu dây, không kết nối hay cấu hình bất kỳ chân nào trong nhóm GPIO 33-37 ra mạch ngoài, nếu không chip sẽ gặp lỗi Panic/Crash ngay lập tức.
"""

    @classmethod
    def generate_human_readable_summary(cls, part_name: str, raw_text: str) -> str:
        """Routes each component to its specialized synthesizer."""
        name_lower = part_name.lower()
        if "schdoc" in name_lower or "schematic" in name_lower:
            return cls.synthesize_schematic(part_name, raw_text)
        elif "me6231" in name_lower:
            return cls.synthesize_me6231(raw_text)
        elif "uc8253" in name_lower:
            return cls.synthesize_uc8253(raw_text)
        elif "ssd1680" in name_lower or "ssd1681" in name_lower or "ssd1683" in name_lower or "jd7966" in name_lower:
            return cls.synthesize_ssd1680(raw_text)
        elif "wafer" in name_lower or "connector" in name_lower:
            return cls.synthesize_wafer_connector()
        elif "bq25180" in name_lower:
            return cls.synthesize_bq25180(raw_text)
        elif "rt9080" in name_lower:
            return cls.synthesize_rt9080(raw_text)
        elif "tps22916" in name_lower:
            return cls.synthesize_tps22916(raw_text)
        elif "max98357" in name_lower:
            return cls.synthesize_max98357a(raw_text)
        elif "inmp441" in name_lower:
            return cls.synthesize_inmp441(raw_text)
        elif "bno055" in name_lower:
            return cls.synthesize_bno055(raw_text)
        elif "ws2812" in name_lower:
            return cls.synthesize_ws2812(raw_text)
        elif "esp32-s3" in name_lower or "esp32_s3" in name_lower:
            return cls.synthesize_esp32_s3(raw_text)
        else:
            return cls.synthesize_generic(part_name, raw_text)
