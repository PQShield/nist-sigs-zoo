"""Compare regenerated ICCS KAT records with a submitted KAT file.

Canonical form: CR and trailing blanks stripped, empty lines dropped, upper case
(hex case varies between submissions). When the submitted file was generated in
the ICCS template mode (blank "X_Len =" lines), every *_Len line is dropped from
both sides. Only the first `count` records are compared.
"""

import re

LEN_LINE = re.compile(r"^\w+_len\s*=", re.IGNORECASE)
BLANK_LEN = re.compile(r"^\w+_len\s*=$", re.IGNORECASE)


def canonical(text: str, nolen: bool) -> list[str]:
    lines = []
    for raw in text.splitlines():
        line = raw.rstrip().upper()
        if line and not (nolen and LEN_LINE.match(line)):
            lines.append(line)
    return lines


def records(lines: list[str]) -> list[list[str]]:
    out: list[list[str]] = []
    for line in lines:
        if line.startswith("COUNT ="):
            out.append([])
        if out:
            out[-1].append(line)
    return out


def compare(generated: str, reference: str, count: int) -> str:
    nolen = any(BLANK_LEN.match(raw.rstrip()) for raw in reference.splitlines())
    gen = records(canonical(generated, nolen))[:count]
    ref = records(canonical(reference, nolen))[:count]
    if len(gen) < count or len(ref) < count:
        return "MISMATCH"
    return "PASS" if gen == ref else "MISMATCH"
