# NGCC build triage: symptom → likely cause in our row

Check the "our row" causes first. Treat a failure as a submission defect only when the row matches the submitter's default build and the failure still reproduces. In that case note it in the internal `docs/ngcc-submission-bugs.md` (local only, git-excluded: never commit it or link to it from committed files).

| Symptom (status.tsv / bench) | Likely cause in our row | Fix |
|---|---|---|
| MISMATCH from the first record, Seed matches | randomness not from `drng_algorithm`: switch missing (`NIST_RAND`, `API_PKC_KAT_DRNG_BRIDGE`, `SUBMISSION_DRNG`) or `/dev/urandom` `randombytes.c` compiled in | set the Makefile's switch; drop the OS-RNG source or patch it to call the DRNG |
| MISMATCH, Seed differs | candidate's own DRNG (e.g. SHAKE `drng.c`) differs from the ICCS SM3 DRNG that generated the KAT | check which DRNG the KAT assumes; if it is ICCS, use an ICCS `drng.c` copy in `patches/<id>/` |
| MISMATCH, PK differs after the seed part | hash/XOF selector missing (`XOF_PSEUDO`, `USE_ICCS`) or wrong backend sources | mirror the Makefile's selector |
| MISMATCH on an opt row only | compared against the reference KAT | point `kat` at the opt tree's own KAT file |
| PASS but sizes equal to another instance's | the parameter selector is missing, so every row built the default set | add the per-row `-DSECURITY_LEVEL=` / `-DPARAMS=` / `-D<X>_MODE=` |
| opt sizes differ from ref | different parameter set or a defect in the opt headers | keep ref; note why |
| CRASH (SIGABRT) in keygen/sign | debug `assert` still on | `-DNDEBUG` if the submitter's default build sets it; otherwise the assert is a finding |
| CRYPTOFAIL / valid sig rejected | compiler-flag-dependent UB, or a buffer-length misreport in the ICCS wrapper | check the documented flags; fix the length in a patched wrapper copy |
| LOADFAIL / undefined symbol | `*` does not descend into subdirectories; C++ sources not compiled | list the subdir sources and `incs` explicitly; C++ trees stay ref |
| link error `relocation ... can not be used when making a shared object` | asm reads constant tables PC-relative | compile the consts `.c` through a hidden-visibility wrapper in `patches/<id>/` (`#pragma GCC visibility push(hidden)` / `#include` / `pop`) |
| duplicate symbol `crypto_sign_*` / `crypto_kem_*` | candidate defines NIST-API names that the shim exports | `defs` `-Dcrypto_sign_keypair=cand_crypto_sign_keypair` etc. |
| TIMEOUT | the KAT replay is slow, or a missing fast-path define (e.g. `SBOX_TABLE`) | add the define first; then raise `NGCC_KAT_TIMEOUT` / lower `katn`, and record either in a comment |
| NOKAT | KAT is named by level rather than instance, or is in a different directory | search `Test_Vectors/` and the tree; NOKAT only if none ships |
| timing >5× slower than published per-op cycles | Makefile switches missing (`SBOX_TABLE`, AES-NI/PCLMUL, `PRG_ACCE`, `NDEBUG`) | mirror the Makefile, then re-run status and sizes |
| status run killed / container OOM | too many rows in one run | filter `PARAMS` to one candidate group |
| bench process dies, later rows missing | a scheme aborts | DNF rows isolate this now; the crash is fixed in the candidate's wrapper, never in the harness |
