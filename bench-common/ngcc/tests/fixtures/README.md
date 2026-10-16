# Synthetic NGCC test candidates

`sign-99` (Toy-Sig-1) and `kem-99` (Toy-KEM-1) are **not** real submissions. They are tiny
ICCS-style candidates written for our tests, laid out like a real round-1 archive
(`Reference_Implementation/<instance>/…`, `KAT/KAT_*.txt`), with KAT files we generated
ourselves. `Toy-Sig-1/cxx/toy_tag.cpp` is a C++ copy of the toy's keyed mix: the
`NGCC-Toy-Sig-Cxx` row builds it (with `-DTOY_CXX`) to test C++ compilation and linking of the
C++ runtime; it reproduces the same KAT.

They let the Python tests exercise the whole NGCC pipeline (rule generation, shim build,
KAT replay, `status.py`, DNF handling) on the host and in CI without compiling any real,
untrusted candidate code, which only ever runs inside `scripts/ngcc-run`. The rule against
committing KAT files is about the real submissions' test vectors (replaced by
`bench-common/ngcc/kat_hashes.tsv`); these toy vectors are ours.
