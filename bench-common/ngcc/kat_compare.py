"""Canonical KAT records and their hashes; compare our ngcc_kat output with them.

The submitted KAT files are not needed at status time: kat_hashes.py stores a
sha256 per canonical record of each submitted file in kat_hashes.tsv, and
status.py compares the records our ngcc_kat replay writes against those.

Canonical form (applied identically to submitted files and to our output):
  1. Decode as UTF-8 (invalid bytes replaced) and split into lines (\\n, \\r\\n, \\r).
  2. Remove every whitespace character from each line.
  3. Lower-case the line: field names (`PK`, `pk`) and hex digits alike.
  4. Drop empty lines and length lines, i.e. lines whose field name (the text
     before the first '=') ends in `_len`. Lengths are implied by the hex values,
     and submissions in the ICCS template mode leave them blank.
  5. A record starts at each line whose field name is `count`; lines before the
     first record (headers) are dropped.
  6. The record's canonical bytes are its lines, each followed by '\\n', in UTF-8.
A record hash is the lower-case hex sha256 of those bytes.

Usage (diagnosis, with a submitted file from `extract.py --kats`):
    python3 -I kat_compare.py GENERATED.txt SUBMITTED.txt [records]
"""

import hashlib
import sys
from collections.abc import Iterable, Iterator

MAX_RECORDS = 10  # ngcc_kat replays at most 10 records


def canonical_line(raw: str) -> str | None:
    line = "".join(raw.split()).lower()
    if not line or line.split("=", 1)[0].endswith("_len"):
        return None
    return line


def canonical_records(lines: Iterable[str], limit: int | None = None) -> Iterator[str]:
    """Yield the canonical records (see module doc) of an iterable of text chunks."""
    current: list[str] | None = None
    emitted = 0
    for chunk in lines:
        for raw in chunk.splitlines():
            line = canonical_line(raw)
            if line is None:
                continue
            if line.split("=", 1)[0] == "count":
                if current is not None:
                    yield "".join(f"{x}\n" for x in current)
                    emitted += 1
                    if limit is not None and emitted >= limit:
                        return
                current = []
            if current is not None:
                current.append(line)
    if current is not None and (limit is None or emitted < limit):
        yield "".join(f"{x}\n" for x in current)


def record_hash(record: str) -> str:
    return hashlib.sha256(record.encode("utf-8")).hexdigest()


def record_hashes(text: str, limit: int | None = MAX_RECORDS) -> list[str]:
    return [record_hash(r) for r in canonical_records([text], limit)]


def check(generated: str, expected: list[str], count: int) -> tuple[str, int | None]:
    """PASS, or MISMATCH and the index of the first record that differs or is missing."""
    got = record_hashes(generated, count)
    for i in range(count):
        if i >= len(got) or i >= len(expected) or got[i] != expected[i]:
            return "MISMATCH", i
    return "PASS", None


def compare(generated: str, reference: str, count: int) -> str:
    return check(generated, record_hashes(reference, count), count)[0]


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 3):
        print(__doc__, file=sys.stderr)
        return 2
    count = int(argv[2]) if len(argv) == 3 else MAX_RECORDS
    with open(argv[0], encoding="utf-8", errors="replace") as f:
        gen = list(canonical_records(f, count))
    with open(argv[1], encoding="utf-8", errors="replace") as f:
        ref = list(canonical_records(f, count))
    status, first = check("".join(gen), [record_hash(r) for r in ref], count)
    print(status if first is None else f"{status} at record {first}")
    if first is not None:
        ours = gen[first].splitlines() if first < len(gen) else []
        theirs = ref[first].splitlines() if first < len(ref) else []
        for i in range(max(len(ours), len(theirs))):
            a = ours[i] if i < len(ours) else "<none>"
            b = theirs[i] if i < len(theirs) else "<none>"
            if a != b:
                print(f"  ours:      {a[:120]}\n  submitted: {b[:120]}")
                break
    return 0 if first is None else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
