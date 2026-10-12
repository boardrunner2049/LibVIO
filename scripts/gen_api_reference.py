#!/usr/bin/env python3
"""Generate contract/api_reference.md and contract/api_summary.md from the doc
comments in include/vio/*.h.

A `/** ... */` comment directly above a declaration documents it; one
followed by a blank line introduces the header. Declarations without a doc
comment are internal and left out. A comment line `@signature <declaration>`
shows that declaration instead of the code below it, for a macro that
callers use like a function.

Both files hold the same text: api_reference.md laid out for reading,
api_summary.md with one line per function, for prompts.

    scripts/gen_api_reference.py           # rewrite both files
    scripts/gen_api_reference.py --check   # fail if either is out of date
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INCLUDE = ROOT / "include" / "vio"
OUTPUT = ROOT / "contract" / "api_reference.md"
SUMMARY = ROOT / "contract" / "api_summary.md"

# Reading order (umbrella header first, then from simple I/O to buses),
# section titles, and who each section is for: "model" (device models),
# "slave" (slave models on a LibVIO bus) or "host" (the emulator loading
# them). The audience is written into the section as <!-- for: ... --> so
# tools that put the reference into a prompt can pick sections.
HEADERS = {
    "vio.h": (None, None),
    "log.h": ("Logging", "host"),
    "pty.h": ("Serial ports (pseudo-terminals)", "model"),
    "fifo.h": ("Named pipes", "model"),
    "fileio.h": ("Host files", "model"),
    "net.h": ("Network (TAP)", "model"),
    "signals.h": ("Signal lines between models", "model"),
    "dma.h": ("DMA requests between models", "model"),
    "adc.h": ("ADC inputs", "model"),
    "spi.h": ("SPI buses", "model"),
    "i2c.h": ("I2C buses", "model"),
    "slave.h": ("Slave models", "slave"),
}
AUDIENCES = {"model", "slave", "host"}


def clean_comment(lines: list[str]) -> str:
    text = []
    for line in lines:
        line = line.strip()
        line = re.sub(r"^/\*\*\s?", "", line)
        line = re.sub(r"\s*\*/$", "", line)
        line = re.sub(r"^\*( |$)", "", line)
        text.append(line)
    while text and not text[0]:
        text.pop(0)
    while text and not text[-1]:
        text.pop()
    return "\n".join(text)


def read_declaration(lines: list[str], i: int) -> tuple[str, int]:
    """Return the declaration starting at lines[i] and the index after it."""
    first = lines[i].strip()
    if first.startswith("#define"):
        decl = []
        while i < len(lines) and lines[i].strip().startswith("#define"):
            line = lines[i].rstrip()
            if line.endswith("\\"):  # a multi-line macro: show its name only
                decl.append(line[:-1].rstrip())
                while i < len(lines) and lines[i].rstrip().endswith("\\"):
                    i += 1
                return "\n".join(decl), i + 1
            decl.append(line)
            i += 1
        return "\n".join(decl), i
    if first.startswith(("typedef struct", "struct")):
        while not re.search(r"}\s*\w*\s*;", lines[i]):
            i += 1
        name = re.search(r"}\s*(\w*)\s*;", lines[i]).group(1)
        return f"typedef struct {{ ... }} {name};", i + 1
    decl = []
    while True:
        line = re.sub(r"//.*", "", lines[i]).rstrip()
        decl.append(line)
        i += 1
        if line.endswith(";") or line.endswith("};"):
            break
    if decl[0].lstrip().startswith("enum"):
        return "\n".join(decl), i
    return re.sub(r"\s+", " ", " ".join(decl)).replace("( ", "(").strip(), i


def parse_header(path: Path) -> tuple[str, list[tuple[str, str]]]:
    lines = path.read_text().splitlines()
    intro, entries = "", []
    i = 0
    while i < len(lines):
        if not lines[i].strip().startswith("/**"):
            i += 1
            continue
        start = i
        while "*/" not in lines[i]:
            i += 1
        doc = clean_comment(lines[start:i + 1])
        i += 1
        if i >= len(lines) or not lines[i].strip():
            intro = doc if not intro else intro + "\n\n" + doc
            continue
        decl, i = read_declaration(lines, i)
        signature = re.search(r"^@signature (.+)$", doc, re.MULTILINE)
        if signature:
            decl = signature.group(1).strip()
            doc = (doc[:signature.start()] + doc[signature.end():]).strip()
        entries.append((decl, doc))
    return intro, entries


def check_headers() -> None:
    """Every public header needs a title and an audience in HEADERS."""
    missing = sorted(p.name for p in INCLUDE.glob("*.h") if p.name not in HEADERS)
    if missing:
        raise SystemExit(f"add {', '.join(missing)} to HEADERS in {Path(__file__).name} "
                         "(section title and audience)")
    for name, (title, audience) in HEADERS.items():
        if title and audience not in AUDIENCES:
            raise SystemExit(f"{name}: audience must be one of {sorted(AUDIENCES)}, not {audience!r}")


def _sections():
    check_headers()
    for name, (title, audience) in HEADERS.items():
        intro, entries = parse_header(INCLUDE / name)
        yield name, title, audience, intro, entries


def _one_line(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def generate_reference(version: str) -> str:
    out = [
        "# LibVIO API reference",
        "",
        "<!-- Generated by scripts/gen_api_reference.py from include/vio/*.h. Do not edit. -->",
        "",
        f"Contract version {version}. Functions that the emulator itself provides to models",
        "(guest memory, registers, interrupts, timers) are described in `host_api.md`.",
        "",
    ]
    for name, title, audience, intro, entries in _sections():
        if title:
            out += [f"## {title}: `<vio/{name}>`", f"<!-- for: {audience} -->", ""]
        out += [intro, ""]
        for decl, doc in entries:
            out += ["```c", decl, "```", "", doc, ""]
    return "\n".join(out).rstrip() + "\n"


def generate_summary(version: str) -> str:
    out = [
        "# LibVIO API",
        "",
        # The version stays in a comment: tools strip comments from prompts, so
        # a new contract version does not change the prompts.
        "<!-- Generated by scripts/gen_api_reference.py from include/vio/*.h. Do not edit."
        f" The same text as api_reference.md, one line per function. Contract version {version}. -->",
        "",
        "Functions that the emulator itself provides to models",
        "(guest memory, registers, interrupts, timers) are listed with the host functions.",
        "",
    ]
    for name, title, audience, intro, entries in _sections():
        if title:
            out += [f"## {title}: `<vio/{name}>`", f"<!-- for: {audience} -->"]
        prose = re.sub(r"```.*?```", "", intro, flags=re.DOTALL)
        out += ["\n\n".join(_one_line(p) for p in prose.split("\n\n") if p.strip()), ""]
        for decl, doc in entries:
            if "\n" in decl:
                out += ["```c", decl, "```", _one_line(doc), ""]
            else:
                out += [f"- `{decl}`: {_one_line(doc)}"]
        out += [""]
    text = "\n".join(out)
    return re.sub(r"\n{3,}", "\n\n", text).rstrip() + "\n"


def generate() -> dict[Path, str]:
    version = (ROOT / "contract" / "VERSION").read_text().strip()
    return {OUTPUT: generate_reference(version), SUMMARY: generate_summary(version)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail if the file is out of date")
    args = parser.parse_args()

    files = generate()
    if args.check:
        stale = [path for path, text in files.items() if not path.exists() or path.read_text() != text]
        for path in stale:
            print(f"{path.relative_to(ROOT)} is out of date; run scripts/gen_api_reference.py",
                  file=sys.stderr)
        return 1 if stale else 0
    for path, text in files.items():
        path.write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
