"""
Page Router & Classifier for PDF Datasheets.
Selects target pages containing critical technical sections before feeding to Docling.
"""
import re
from pathlib import Path
from typing import Dict, List, Set, Tuple
from pypdf import PdfReader

from .config import SECTION_KEYWORDS


class PageRouter:
    """Scans and routes PDF pages to avoid processing unnecessary non-technical pages."""

    def __init__(self, pdf_path: Path):
        self.pdf_path = Path(pdf_path)
        try:
            self.reader = PdfReader(str(self.pdf_path), strict=False)
            self.total_pages = len(self.reader.pages)
        except Exception:
            self.reader = None
            self.total_pages = 0

    def scan_pages(self, max_threshold: int = 25) -> Tuple[List[int], Dict[str, List[int]]]:
        """
        Returns:
            target_pages: Sorted list of 1-based page indices to process.
            section_pages: Mapping of section name to 1-based page numbers.
        """
        if self.reader is None or self.total_pages == 0:
            return [], {}

        # If document is short, process all pages directly
        if self.total_pages <= max_threshold:
            all_pages = list(range(1, self.total_pages + 1))
            return all_pages, {"all": all_pages}

        section_pages: Dict[str, Set[int]] = {sec: set() for sec in SECTION_KEYWORDS}
        target_pages: Set[int] = set()

        # Always include first 2 pages for metadata/overview
        target_pages.add(1)
        if self.total_pages >= 2:
            target_pages.add(2)

        for page_idx, page in enumerate(self.reader.pages, start=1):
            try:
                text = page.extract_text() or ""
            except Exception:
                text = ""

            lower_text = text.lower()
            for sec_name, patterns in SECTION_KEYWORDS.items():
                for pat in patterns:
                    if re.search(pat, lower_text):
                        section_pages[sec_name].add(page_idx)
                        target_pages.add(page_idx)
                        # Datasheet sections often span 1-2 subsequent pages
                        if page_idx + 1 <= self.total_pages:
                            target_pages.add(page_idx + 1)
                        break

        sorted_targets = sorted(list(target_pages))
        converted_sections = {k: sorted(list(v)) for k, v in section_pages.items() if v}

        return sorted_targets, converted_sections
