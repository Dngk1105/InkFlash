"""
Docling Engine: Hybrid wrapper supporting IBM Docling (deep layout)
and pdfplumber (fast local table extraction).
"""
from pathlib import Path
from typing import List, Optional, Tuple
import pandas as pd

try:
    from docling.datamodel.base_models import InputFormat
    from docling.datamodel.pipeline_options import PdfPipelineOptions
    from docling.document_converter import DocumentConverter, PdfFormatOption
    DOCLING_AVAILABLE = True
except ImportError:
    DOCLING_AVAILABLE = False

try:
    import pdfplumber
    PDFPLUMBER_AVAILABLE = True
except ImportError:
    PDFPLUMBER_AVAILABLE = False


class DoclingEngine:
    """Manages document conversion via IBM Docling with pdfplumber fallback."""

    def __init__(self, do_ocr: bool = False, prefer_docling: bool = True):
        self.do_ocr = do_ocr
        self.prefer_docling = prefer_docling and DOCLING_AVAILABLE

    def convert_pdf(
        self,
        pdf_path: Path,
        page_range: Optional[List[int]] = None
    ) -> Tuple[str, List[str]]:
        """
        Converts the PDF (or specific pages) to Markdown and extracts tables.
        """
        if self.prefer_docling:
            try:
                return self._convert_with_docling(pdf_path, page_range)
            except Exception as e:
                if PDFPLUMBER_AVAILABLE:
                    return self._convert_with_pdfplumber(pdf_path, page_range)
                raise e

        if PDFPLUMBER_AVAILABLE:
            return self._convert_with_pdfplumber(pdf_path, page_range)

        raise ImportError("Neither Docling nor pdfplumber is available in this environment.")

    def _convert_with_docling(
        self,
        pdf_path: Path,
        page_range: Optional[List[int]] = None
    ) -> Tuple[str, List[str]]:
        pipeline_options = PdfPipelineOptions()
        pipeline_options.do_ocr = self.do_ocr
        pipeline_options.do_table_structure = True

        if page_range:
            pipeline_options.page_range = page_range

        converter = DocumentConverter(
            format_options={
                InputFormat.PDF: PdfFormatOption(pipeline_options=pipeline_options)
            }
        )

        result = converter.convert(str(pdf_path))
        markdown_text = result.document.export_to_markdown()

        tables_md = []
        for table in getattr(result.document, "tables", []):
            try:
                tables_md.append(table.export_to_markdown())
            except Exception:
                pass

        return markdown_text, tables_md

    def _convert_with_pdfplumber(
        self,
        pdf_path: Path,
        page_range: Optional[List[int]] = None
    ) -> Tuple[str, List[str]]:
        """Fast fallback extractor using pdfplumber."""
        extracted_sections = []
        tables_md = []

        with pdfplumber.open(str(pdf_path)) as pdf:
            total_pages = len(pdf.pages)
            target_indices = (
                [p - 1 for p in page_range if 1 <= p <= total_pages]
                if page_range
                else list(range(total_pages))
            )

            for idx in target_indices:
                page = pdf.pages[idx]
                page_text = page.extract_text() or ""
                tables = page.extract_tables()

                page_md = [f"## Page {idx + 1}\n"]

                if tables:
                    for t_idx, table in enumerate(tables):
                        if not table or len(table) < 2:
                            continue
                        clean_rows = []
                        for row in table:
                            clean_rows.append([str(c or "").replace("\n", " ").strip() for c in row])

                        header = clean_rows[0]
                        data = clean_rows[1:]
                        try:
                            df = pd.DataFrame(data, columns=header)
                            # Remove completely empty columns
                            df = df.loc[:, ~(df.columns == "")]
                            table_markdown = df.to_markdown(index=False)
                            tables_md.append(table_markdown)
                            page_md.append(f"\n{table_markdown}\n")
                        except Exception:
                            pass

                if page_text:
                    page_md.append(f"\n{page_text.strip()}\n")

                extracted_sections.append("\n".join(page_md))

        return "\n\n---\n\n".join(extracted_sections), tables_md
