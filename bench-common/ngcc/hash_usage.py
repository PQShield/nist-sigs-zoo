"""Classify how an NGCC instance hashes, by scanning the sources it is built from.

The ICCS package ships auxfunc.c with sm3hash (a real SM3) and two placeholders,
pseudohash and pseudoXOF, that ICCS documents as "only used for correctness
verification ... does not guarantee security". Timings of an instance that calls
the placeholders include them, which the site notes per parameter set.

  placeholder  some source calls pseudohash() or pseudoXOF()
  sm3          only sm3hash() from the ICCS package is called
  own          neither (the candidate brings its own hash/XOF)
"""

import re
from collections.abc import Iterable
from pathlib import Path

CALL = re.compile(r"\b(pseudohash|pseudoXOF|sm3hash)\s*\(")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
AUX_FILES = {"auxfunc.c", "auxfunc.h"}


def hash_usage(paths: Iterable[Path]) -> str:
    found: set[str] = set()
    for p in paths:
        if p.name in AUX_FILES or not p.is_file():
            continue
        text = COMMENTS.sub(" ", p.read_text(errors="replace"))
        found.update(m.group(1) for m in CALL.finditer(text))
    if found & {"pseudohash", "pseudoXOF"}:
        return "placeholder"
    return "sm3" if "sm3hash" in found else "own"
