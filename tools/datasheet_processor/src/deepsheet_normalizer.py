"""
DeepSheet Normalizer: Cleans, formats, and restructures extracted content
into standardized GFM Markdown for electronics hardware engineers.
"""
import re
from typing import List
from .config import UNIT_REPLACEMENTS, ACTIVE_LOW_PATTERN


class DeepSheetNormalizer:
    """Normalizes units, electrical symbols, active-low signals, and Markdown formatting."""

    @staticmethod
    def normalize_units_and_symbols(text: str) -> str:
        """Applies engineering unit and symbol replacements."""
        for pattern, replacement in UNIT_REPLACEMENTS:
            text = pattern.sub(replacement, text)
        return text

    @staticmethod
    def normalize_active_low_pins(pin_name: str) -> str:
        """Standardizes active-low signal notation to SIGNAL_N."""
        match = ACTIVE_LOW_PATTERN.match(pin_name.strip())
        if match:
            core = match.group(1).upper()
            return f"{core}_N"
        return pin_name

    @staticmethod
    def cleanup_markdown_table(table_md: str) -> str:
        """Aligns, cleans headers, and fixes merged cell artifacts in Markdown tables."""
        lines = [line.strip() for line in table_md.strip().split("\n") if line.strip()]
        if not lines or "|" not in lines[0]:
            return table_md

        cleaned_lines: List[str] = []
        for line in lines:
            # Replace multiple consecutive pipes with standard column separator
            line = re.sub(r"\|{2,}", "|", line)
            # Apply electronics unit normalization to cell content
            line = DeepSheetNormalizer.normalize_units_and_symbols(line)
            cleaned_lines.append(line)

        return "\n".join(cleaned_lines)

    @classmethod
    def structure_datasheet_document(
        cls,
        part_name: str,
        raw_markdown: str,
        section_tables: dict = None
    ) -> str:
        """
        Organizes raw markdown into the standardized DeepSheet component template.
        """
        normalized_content = cls.normalize_units_and_symbols(raw_markdown)

        # Highlight Absolute Maximum Ratings with GitHub Alert
        if "absolute maximum ratings" in normalized_content.lower():
            normalized_content = re.sub(
                r"(#+\s*Absolute\s+Maximum\s+Ratings.*?\n)",
                r"\1\n> [!CAUTION]\n> Stresses beyond those listed under Absolute Maximum Ratings may cause permanent damage.\n\n",
                normalized_content,
                flags=re.IGNORECASE,
            )

        # Add header banner
        header = f"""# {part_name.upper()} - Hardware Technical Specification

> **Extracted via:** DeepSheet + Docling Pipeline  
> **Source Document:** {part_name}.pdf  

---

"""
        return header + normalized_content
