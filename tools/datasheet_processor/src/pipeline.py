"""
Master Pipeline: Orchestrates Page Routing, Docling Layout Parsing, and DeepSheet Normalization.
"""
import sys
from pathlib import Path
from typing import Optional
from rich.console import Console

if sys.stdout and hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

from .page_router import PageRouter
from .docling_engine import DoclingEngine
from .deepsheet_normalizer import DeepSheetNormalizer
from .synthesizer import DatasheetSynthesizer

console = Console(highlight=False)


class DatasheetPipeline:
    """End-to-end extraction pipeline for electronic component datasheets."""

    def __init__(self, do_ocr: bool = False, max_direct_pages: int = 25):
        self.do_ocr = do_ocr
        self.max_direct_pages = max_direct_pages
        self.engine = DoclingEngine(do_ocr=do_ocr)

    def process_file(
        self,
        pdf_path: Path,
        output_dir: Path,
        force_all_pages: bool = False
    ) -> Path:
        """Processes a single PDF file and saves the normalized markdown output."""
        pdf_path = Path(pdf_path)
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        safe_name = pdf_path.name.encode("ascii", "replace").decode("ascii")
        part_name = pdf_path.stem
        console.print(f"[bold cyan]Scanning:[/bold cyan] {safe_name}")

        router = PageRouter(pdf_path)
        console.print(f"Total pages: [yellow]{router.total_pages or 'unknown'}[/yellow]")

        page_range = None
        if router.total_pages > 0 and not force_all_pages and router.total_pages > self.max_direct_pages:
            targets, sections = router.scan_pages(max_threshold=self.max_direct_pages)
            if targets:
                console.print(
                    f"Smart routing selected [green]{len(targets)}[/green] key technical pages: "
                    f"{targets[:10]}{'...' if len(targets) > 10 else ''}"
                )
                page_range = targets
        else:
            console.print(f"Processing all pages directly...")

        console.print("[cyan]Running IBM Docling layout and table parsing...[/cyan]")
        raw_md, tables = self.engine.convert_pdf(pdf_path, page_range=page_range)

        console.print(f"[cyan]Applying DeepSheet normalization & human-readable synthesis...[/cyan]")
        normalized_md = DeepSheetNormalizer.structure_datasheet_document(
            part_name=part_name,
            raw_markdown=raw_md
        )

        human_readable_md = DatasheetSynthesizer.generate_human_readable_summary(
            part_name=part_name,
            raw_text=raw_md
        )

        # Save both human readable overview and full dump
        out_file = output_dir / f"{part_name}.md"
        out_file.write_text(human_readable_md, encoding="utf-8")

        full_file = output_dir / f"{part_name}_full.md"
        full_file.write_text(normalized_md, encoding="utf-8")

        console.print(f"[bold green]Saved clean summary to:[/bold green] {out_file}")
        return out_file

    def process_directory(
        self,
        input_dir: Path,
        output_dir: Path,
        force_all_pages: bool = False
    ) -> list:
        """Processes all PDF files inside a directory."""
        input_dir = Path(input_dir)
        pdf_files = list(input_dir.glob("*.pdf"))
        if not pdf_files:
            console.print(f"[yellow]No PDF files found in {input_dir}[/yellow]")
            return []

        console.print(f"[bold]Found {len(pdf_files)} PDF files to process.[/bold]\n")
        outputs = []
        for pdf in pdf_files:
            try:
                out = self.process_file(pdf, output_dir, force_all_pages=force_all_pages)
                outputs.append(out)
            except Exception as e:
                console.print(f"[bold red]Failed to process {pdf.name}:[/bold red] {e}\n")

        return outputs
