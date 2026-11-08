# Design: docs/writing-comments.md
"""Check terminology and comment rules; rename identifiers from docs/glossary.yaml.

    python tools/terms.py check [paths...]      # avoid words, comment language, tags, Doxygen
    python tools/terms.py rename [--base REF]   # dry-run: diff glossary vs REF (default HEAD), print changes
    python tools/terms.py rename --apply        # write changes, git mv renamed files

Run in the ESP-IDF terminal: needs PyYAML (present in the ESP-IDF venv).
"""
import argparse
import difflib
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GLOSSARY = "docs/glossary.yaml"
TEXT_EXT = (".c", ".h", ".md", ".py", ".txt", ".cmake", ".yaml", ".yml", ".json", ".html", ".js")
CODE_EXT = (".c", ".h")

# Tags listed in docs/writing-comments.md and in TODO Tree (.vscode/settings.json). Keep the three lists in sync.
TAGS = ("TODO", "FIXME", "BUG", "HACK", "PERF", "REVIEW", "EXTEND", "WARN", "NOTE")
TAG_RE = re.compile(r"(?<![@\w])(" + "|".join(TAGS) + r")\b(\([\w./-]+\))?(:)?")
# Vietnamese letters; × (U+00D7) and ÷ (U+00F7) are math, allowed.
VIET_RE = re.compile(r"[À-ÖØ-öø-ɏḀ-ỿ]")
IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


# ---- Glossary ----

def load_yaml(text):
    try:
        import yaml
    except ImportError:
        sys.exit("terms.py: missing PyYAML. Run in the ESP-IDF terminal (venv has PyYAML).")
    return yaml.safe_load(text) or {}


def load_glossary(ref=None):
    if ref is None:
        with open(os.path.join(ROOT, GLOSSARY), encoding="utf-8") as f:
            return load_yaml(f.read())
    r = subprocess.run(["git", "show", f"{ref}:{GLOSSARY}"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8")
    if r.returncode != 0:
        sys.exit(f"terms.py: {GLOSSARY} not found at {ref}. Commit the glossary first.")
    return load_yaml(r.stdout)


def by_id(items, kind):
    out = {}
    for item in items or []:
        if "id" not in item:
            sys.exit(f"terms.py: {kind} entry without id: {item}")
        if item["id"] in out:
            sys.exit(f"terms.py: duplicate {kind} id: {item['id']}")
        out[item["id"]] = item
    return out


# ---- Files ----

def tracked_files():
    r = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", check=True)
    return [p for p in r.stdout.split("\0") if p]


def is_text(rel):
    return rel.endswith(TEXT_EXT) or os.path.basename(rel) == "CMakeLists.txt"


def read(rel):
    """File text, or None if not UTF-8 (vendor sources, data)."""
    try:
        with open(os.path.join(ROOT, rel), encoding="utf-8", newline="") as f:
            return f.read()
    except UnicodeDecodeError:
        return None


def in_scope(rel, scope):
    return any(rel == s or rel.startswith(s.rstrip("/") + "/") for s in scope)


def line_of(text, pos):
    return text.count("\n", 0, pos) + 1


# ---- C comments ----

def comment_spans(text):
    """(start, end) of every comment in C source; skips string and char literals."""
    spans, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            i = j + 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            spans.append((i, j))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            spans.append((i, j))
            i = j
        else:
            i += 1
    return spans


def prose_spans(rel, text):
    """Parts of a file written as prose: whole .md file, comments of .c/.h."""
    if rel.endswith(".md"):
        return [(0, len(text))]
    if rel.endswith(CODE_EXT):
        return comment_spans(text)
    return []


# ---- check ----

def phrase_re(phrase):
    return re.compile(r"(?<!\w)" + re.escape(phrase) + r"(?!\w)", re.IGNORECASE)


def check_avoid(rel, text, avoid, errors):
    for start, end in prose_spans(rel, text):
        chunk = text[start:end]
        for rx, phrase, term in avoid:
            for m in rx.finditer(chunk):
                errors.append(f"{rel}:{line_of(text, start + m.start())}: '{phrase}' -> dùng '{term}'")


def check_comment_language(rel, text, errors):
    for start, end in comment_spans(text):
        m = VIET_RE.search(text, start, end)
        if m:
            errors.append(f"{rel}:{line_of(text, m.start())}: comment phải viết tiếng Anh (gặp '{m.group(0)}')")


def check_tags(rel, text, errors):
    for start, end in comment_spans(text):
        for m in TAG_RE.finditer(text, start, end):
            tag, _scope, colon = m.groups()
            where = f"{rel}:{line_of(text, m.start())}"
            if not colon:  # without ':' TODO Tree still lists it, but the text reads as a plain word
                errors.append(f"{where}: tag thiếu ':'; viết '{tag}: ...' hoặc '{tag}(scope): ...'")


DECL_RE = re.compile(r"^[A-Za-z_\[][^;{}()#]*?\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*;", re.M)


def check_doxygen(rel, text, errors):
    """Public header: each function prototype has /** ... */ with @brief; error-returning ones have @retval."""
    if "/include/" not in "/" + rel or not rel.endswith(".h"):
        return
    spans = comment_spans(text)
    code = list(text)
    for s, e in spans:
        for k in range(s, e):
            if code[k] != "\n":
                code[k] = " "
    code = "".join(code)
    depth = 0
    depth_at = []
    for ch in code:
        depth_at.append(depth)
        depth += (ch == "{") - (ch == "}")
    for m in DECL_RE.finditer(code):
        if depth_at[m.start()] != 0 or m.group(0).lstrip().startswith("typedef"):
            continue
        before = text[:m.start()].rstrip()
        doc = next((text[s:e] for s, e in reversed(spans) if e == len(before)), None)
        where = f"{rel}:{line_of(text, m.start())}: {m.group(1)}()"
        if not doc or not doc.startswith("/**"):
            errors.append(f"{where}: thiếu comment /** */ ngay trên khai báo")
            continue
        if "@brief" not in doc:
            errors.append(f"{where}: thiếu @brief")
        returns_error = re.search(r"\b(esp_err_t|\w+_error_t)\b", m.group(0).split(m.group(1))[0])
        if returns_error and "@retval" not in doc:
            errors.append(f"{where}: trả mã lỗi nhưng thiếu @retval")


def run_check(paths=None):
    """Return error lines. paths: repo-relative; None = glossary scope."""
    g = load_glossary()
    scope = g.get("scope") or []
    avoid = []
    for t in g.get("terms") or []:
        for phrase in t.get("avoid") or []:
            avoid.append((phrase_re(phrase), phrase, t["term"]))
    avoid.sort(key=lambda a: -len(a[1]))
    files = [p for p in tracked_files() if is_text(p) and p != GLOSSARY]
    if paths:
        norm = [os.path.relpath(os.path.abspath(p), ROOT).replace(os.sep, "/") for p in paths]
        files = [p for p in files if in_scope(p, norm)]
    else:
        files = [p for p in files if in_scope(p, scope)]
    errors = []
    for rel in files:
        text = read(rel)
        if text is None:
            continue
        check_avoid(rel, text, avoid, errors)
        if rel.endswith(CODE_EXT):
            check_comment_language(rel, text, errors)
            check_tags(rel, text, errors)
            check_doxygen(rel, text, errors)
    return errors


# ---- rename ----

def term_tokens(term):
    return [w for w in re.split(r"[\s_/-]+", term.strip()) if w]


def derive_symbol(symbol, old_term, new_term):
    """Replace the old term's words inside a snake_case symbol, keeping case."""
    parts = symbol.split("_")
    old = [w.lower() for w in term_tokens(old_term)]
    new = [w.lower() for w in term_tokens(new_term)]
    for i in range(len(parts) - len(old) + 1):
        if [p.lower() for p in parts[i:i + len(old)]] == old:
            upper = all(p.isupper() for p in parts[i:i + len(old)] if p.isalpha())
            repl = [w.upper() if upper else w for w in new]
            return "_".join(parts[:i] + repl + parts[i + len(old):])
    return symbol


def build_plan(old_g, new_g):
    """Return (prefixes, symbols, prose, notes)."""
    prefixes, symbols, prose, notes = {}, {}, {}, []
    old_m, new_m = by_id(old_g.get("modules"), "module"), by_id(new_g.get("modules"), "module")
    for mid, om in old_m.items():
        nm = new_m.get(mid)
        if nm and nm["prefix"] != om["prefix"]:
            prefixes[om["prefix"]] = nm["prefix"]
    old_t, new_t = by_id(old_g.get("terms"), "term"), by_id(new_g.get("terms"), "term")
    for tid, ot in old_t.items():
        nt = new_t.get(tid)
        if not nt:
            continue
        os_, ns_ = ot.get("symbols") or [], nt.get("symbols") or []
        if len(os_) != len(ns_):
            sys.exit(f"terms.py: term '{tid}': số phần tử symbols đổi ({len(os_)} -> {len(ns_)}); giữ nguyên số phần tử.")
        term_changed = ot["term"] != nt["term"]
        for a, b in zip(os_, ns_):
            if a != b:
                symbols[a] = b
            elif term_changed:
                d = derive_symbol(a, ot["term"], nt["term"])
                if d != a:
                    symbols[a] = d
                else:
                    notes.append(f"term '{tid}': '{a}' không chứa '{ot['term']}', giữ nguyên")
        if term_changed:
            prose[ot["term"]] = nt["term"]
    return prefixes, symbols, prose, notes


def make_ident_mapper(prefixes, symbols, all_prefixes):
    """Map one identifier. Longest known prefix wins at each position."""
    known = sorted(set(all_prefixes) | set(prefixes), key=len, reverse=True)
    forms = []
    for p in known:
        forms.append((p, prefixes.get(p)))
        forms.append((p.upper(), prefixes[p].upper() if p in prefixes else None))

    def mapper(ident):
        ident = symbols.get(ident, ident)  # then prefix: symbol and prefix may change together
        if not prefixes:
            return ident
        out, i = [], 0
        while i < len(ident):
            if i == 0 or ident[i - 1] == "_":
                hit = next(((f, to) for f, to in forms if ident.startswith(f, i)
                            and (i + len(f) == len(ident) or ident[i + len(f)] == "_")), None)
                if hit:
                    f, to = hit
                    out.append(to if to is not None else f)
                    i += len(f)
                    continue
            out.append(ident[i])
            i += 1
        return "".join(out)
    return mapper


def word_case_forms(old, new):
    pairs = [(old, new)]
    if old[:1].islower():
        pairs.append((old[:1].upper() + old[1:], new[:1].upper() + new[1:]))
    return pairs


def rewrite_text(rel, text, mapper, prose):
    # mapper only touches glossary symbols and module prefixes, so plain words are safe.
    text = IDENT_RE.sub(lambda m: mapper(m.group(0)), text)
    if not prose:
        return text
    spans = prose_spans(rel, text)
    if not spans:
        return text
    rules = []
    for old, new in prose.items():
        for o, n in word_case_forms(old, new):
            rules.append((re.compile(r"(?<![\w`])" + re.escape(o) + r"(?![\w`])"), n))
    out, last = [], 0
    for s, e in spans:
        chunk = text[s:e]
        for rx, n in rules:
            chunk = rx.sub(n, chunk)
        out.append(text[last:s])
        out.append(chunk)
        last = e
    out.append(text[last:])
    return "".join(out)


def rename_path(rel, mapper):
    parts = rel.split("/")
    new_parts = []
    for p in parts:
        stem, dot, ext = p.partition(".")
        new_parts.append(mapper(stem) + dot + ext)
    return "/".join(new_parts)


def git_dirty():
    r = subprocess.run(["git", "status", "--porcelain", "-uno"], cwd=ROOT, capture_output=True, text=True, encoding="utf-8")
    return [l[3:] for l in r.stdout.splitlines() if l[3:] != GLOSSARY]


def run_rename(base, apply):
    old_g, new_g = load_glossary(base), load_glossary()
    prefixes, symbols, prose, notes = build_plan(old_g, new_g)
    for n in notes:
        print(f"note: {n}")
    if not (prefixes or symbols or prose):
        print(f"Không có gì đổi: {GLOSSARY} giống bản ở {base}.")
        return 0
    all_prefixes = [m["prefix"] for m in old_g.get("modules") or []]
    mapper = make_ident_mapper(prefixes, symbols, all_prefixes)

    files = [p for p in tracked_files() if is_text(p) and p != GLOSSARY and os.path.isfile(os.path.join(ROOT, p))]
    existing, changes, moves = set(), [], []
    for rel in files:
        text = read(rel)
        if text is None:
            continue
        if rel.endswith(CODE_EXT):
            existing.update(IDENT_RE.findall(text))
        # identifiers: whole repo; prose words: glossary scope only (plain words like "take" match too much)
        new_text = rewrite_text(rel, text, mapper, prose if in_scope(rel, new_g.get("scope") or []) else {})
        if new_text != text:
            changes.append((rel, text, new_text))
    for rel in tracked_files():
        new_rel = rename_path(rel, mapper)
        if new_rel != rel:
            moves.append((rel, new_rel))

    renamed = {i for i in existing if mapper(i) != i}
    clashes = sorted({mapper(i) for i in renamed} & (existing - renamed))
    for kind, table in (("prefix", prefixes), ("symbol", symbols), ("prose", prose)):
        for a, b in table.items():
            print(f"{kind}: {a} -> {b}")
    if clashes:
        print("\nLỗi: tên mới trùng identifier đang có: " + ", ".join(clashes))
        return 1

    for rel, old, new in changes:
        sys.stdout.writelines(difflib.unified_diff(old.splitlines(True), new.splitlines(True), f"a/{rel}", f"b/{rel}", n=0))
    for a, b in moves:
        print(f"git mv {a} {b}")
    print(f"\n{len(changes)} file đổi nội dung, {len(moves)} file đổi tên.")

    if not apply:
        print("Dry-run. Chạy lại với --apply để ghi.")
        return 0
    dirty = git_dirty()
    if dirty:
        print("Lỗi: working tree còn thay đổi ngoài glossary: " + ", ".join(dirty))
        return 1
    for rel, _, new in changes:
        with open(os.path.join(ROOT, rel), "w", encoding="utf-8", newline="") as f:
            f.write(new)
    for a, b in moves:
        os.makedirs(os.path.dirname(os.path.join(ROOT, b)) or ROOT, exist_ok=True)
        subprocess.run(["git", "mv", a, b], cwd=ROOT, check=True)
    print("Đã ghi. Chạy python tools/build_all.py rồi commit glossary cùng code.")
    return 0


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("check")
    c.add_argument("paths", nargs="*")
    r = sub.add_parser("rename")
    r.add_argument("--base", default="HEAD")
    r.add_argument("--apply", action="store_true")
    args = ap.parse_args()
    if args.cmd == "check":
        errors = run_check(args.paths)
        print("\n".join(errors) if errors else "OK: không có lỗi thuật ngữ/comment.")
        sys.exit(1 if errors else 0)
    sys.exit(run_rename(args.base, args.apply))


if __name__ == "__main__":
    main()
