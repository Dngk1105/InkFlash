"""
CLI Interface for Datasheet Processor (Docling + DeepSheet).
"""
import sys
from pathlib import Path
from typing import Optional
import typer
from rich.console import Console

if sys.stdout and hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

from .pipeline import DatasheetPipeline
from .page_router import PageRouter

app = typer.Typer(
    help="AI & heuristic pipeline extracting electronic component datasheets to clean GFM Markdown using Docling and DeepSheet"
)
console = Console(highlight=False)


@app.command()
def extract(
    source: Path = typer.Argument(..., help="Path to a PDF file or directory of PDF files"),
    output: Path = typer.Option(Path("./output_markdown"), "--output", "-o", help="Output directory for Markdown files"),
    ocr: bool = typer.Option(False, "--ocr", help="Enable OCR for scanned/raster PDFs"),
    all_pages: bool = typer.Option(False, "--all-pages", "-a", help="Force processing all pages instead of smart routing"),
):
    """Extract and normalize datasheet technical specifications into GFM Markdown."""
    pipeline = DatasheetPipeline(do_ocr=ocr)

    if source.is_file():
        pipeline.process_file(source, output, force_all_pages=all_pages)
    elif source.is_dir():
        pipeline.process_directory(source, output, force_all_pages=all_pages)
    else:
        console.print(f"[bold red]Error:[/bold red] Source path '{source}' does not exist.")
        raise typer.Exit(code=1)


@app.command()
def inspect(
    pdf_path: Path = typer.Argument(..., help="Path to PDF file to inspect"),
):
    """Quickly inspect a PDF and list pages containing key technical sections."""
    if not pdf_path.exists():
        console.print(f"[bold red]File not found:[/bold red] {pdf_path}")
        raise typer.Exit(code=1)

    router = PageRouter(pdf_path)
    targets, sections = router.scan_pages()

    console.print(f"\n[bold green]Document:[/bold green] {pdf_path.name}")
    console.print(f"Total Pages: [yellow]{router.total_pages}[/yellow]")
    console.print(f"Recommended Target Pages ({len(targets)}): [cyan]{targets}[/cyan]\n")

    console.print("[bold]Detected Sections:[/bold]")
    for sec, pages in sections.items():
        console.print(f"  • [bold]{sec:22}[/bold]: Pages {pages}")
    print()


if __name__ == "__main__":
    app()
