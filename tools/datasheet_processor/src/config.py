"""
Configuration and constants for Electronic Datasheet Processing (DeepSheet + Docling).
"""
import re
from pathlib import Path

# Section matching patterns (case-insensitive)
SECTION_KEYWORDS = {
    "pinout": [
        r"pin\s+configuration",
        r"pin\s+description",
        r"pinout",
        r"terminal\s+functions",
        r"pin\s+assignment",
        r"ball\s+map",
    ],
    "absolute_max": [
        r"absolute\s+maximum\s+ratings",
        r"maximum\s+ratings",
        r"absolute\s+ratings",
    ],
    "operating_conditions": [
        r"recommended\s+operating\s+conditions",
        r"operating\s+conditions",
        r"operating\s+ratings",
    ],
    "electrical_specs": [
        r"electrical\s+characteristics",
        r"dc\s+characteristics",
        r"ac\s+characteristics",
        r"dc\s+electrical\s+characteristics",
        r"ac\s+electrical\s+characteristics",
        r"static\s+characteristics",
        r"dynamic\s+characteristics",
    ],
    "register_map": [
        r"register\s+map",
        r"register\s+summary",
        r"register\s+description",
        r"memory\s+map",
        r"registers",
    ],
    "package": [
        r"package\s+information",
        r"packaging\s+information",
        r"package\s+outline",
        r"ordering\s+information",
    ],
}

# Unit and symbol normalization map
UNIT_REPLACEMENTS = [
    # Greek micro
    (re.compile(r"\b[uU]A\b"), "µA"),
    (re.compile(r"\b[uU]V\b"), "µV"),
    (re.compile(r"\b[uU]F\b"), "µF"),
    (re.compile(r"\b[uU]H\b"), "µH"),
    (re.compile(r"\b[uU]s\b"), "µs"),
    (re.compile(r"\b[uU]m\b"), "µm"),
    (re.compile(r"\b[uU]W\b"), "µW"),
    # Ohms
    (re.compile(r"\bohm\b", re.IGNORECASE), "Ω"),
    (re.compile(r"\bkohm\b", re.IGNORECASE), "kΩ"),
    (re.compile(r"\bMohm\b", re.IGNORECASE), "MΩ"),
    # Temperature
    (re.compile(r"(\d+)\s*(?:deg\s*C|°\s*C|oC)\b", re.IGNORECASE), r"\1 °C"),
    (re.compile(r"(\d+)\s*(?:deg\s*F|°\s*F|oF)\b", re.IGNORECASE), r"\1 °F"),
    # Math & tolerances
    (re.compile(r"\+\s*/\s*-"), "±"),
    (re.compile(r"\bapprox\.?\b", re.IGNORECASE), "≈"),
    # Rails
    (re.compile(r"\bVcc\b"), "$V_{CC}$"),
    (re.compile(r"\bVdd\b"), "$V_{DD}$"),
    (re.compile(r"\bVss\b"), "$V_{SS}$"),
    (re.compile(r"\bVee\b"), "$V_{EE}$"),
    (re.compile(r"\bVref\b"), "$V_{REF}$"),
]

# Active-low pin normalization: RESET#, /RESET, !RESET, nRESET -> RESET_N
ACTIVE_LOW_PATTERN = re.compile(r"(?:[#!\/]|_n\b|\bn)([A-Z0-9_]{2,})(?:#)?", re.IGNORECASE)
