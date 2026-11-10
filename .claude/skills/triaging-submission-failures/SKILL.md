---
name: triaging-submission-failures
description: Use when a submitted candidate implementation (NGCC, NIST on-ramp or similar) fails in our harness — KAT MISMATCH, CRYPTOFAIL, CRASH, TIMEOUT, NOKAT, warm-up "sign failed", a DNF row, timings far from published figures — or before calling something a submission bug, writing it up, or noting it on the site.
---

# Triaging submission failures

**Core principle:** a failure is *our build* until the submitter's own build files say otherwise.

## 1. Rule out our build

Diff our `params.tsv` row against the submitter's Makefile/CMake/README **in the archive** (extraction may omit them):

| Check | Symptom |
|---|---|
| Makefile defines (`-DNIST_RAND`, `-DXOF_PSEUDO`, `-DSECURITY_LEVEL=…`, `-D__AVX2`, AES-NI/PCLMUL/`SBOX_TABLE`) | MISMATCH, CRYPTOFAIL, wrong sizes, 20–100× slow |
| `-DNDEBUG` in the documented build | CRASH on assert, slow |
| KAT for *this* tree (`_OPT`, `_AVX2`, optimised dir) or named by level | MISMATCH, NOKAT |
| Randomness from `drng_algorithm`, not `rand()`/urandom/unseeded DRBG | MISMATCH, same PK every record |
| Sources in subdirs, `.S`, visibility | link/load fail |
| Our driver caps (buffer, timeout) | CRYPTOFAIL on huge sizes, TIMEOUT |
| Identical sizes across levels | default level built |

Timings: compare ref with ref, opt with opt, calibrated. Third-party numbers are cross-checks; never copy them into data.

## 2. Confirm with one container run

Run the corrected row alone via `scripts/ngcc-run`. Intermittent: measure the rate (1000 loops); does the spec allow that return?

## 3. Classify

- **Our build:** fix the row. If the build without the define is unsafe, that is *also* a submission defect.
- **Submission defect:** write it up. To benchmark anyway, patch its wrapper in `patches/<id>/` (PATCH header), never the harness (`shim_template.c`, `ngcc_kat`, `status.py`, `kat_compare`); else it stays DNF/MISMATCH.
- **Already reported:** write it up under "Already reported" with "What we add".
- **Undiagnosed:** list what was tried.

## 4. Security-critical

**Yes** if any holds, however trivial, even when the documented build avoids it:
- fixed, zero, default or predictable key/seed/randomness;
- memory-safety issue, **including in shipped test drivers**;
- default or easy configuration silently weaker.

Severity is separate: Critical = forge/recover as shipped; High = memory/randomness on main path; Medium = specific config, rare input, correctness; Low = packaging, reproducibility.

## 5. Prior reports and credit

Before claiming novelty search ngcc.dev `reports/<id>.html`, pkcforum (`list.niccs.org.cn` HyperKitty search) for candidate names **and** symptom terms, and NIST pqc-forum. Record terms and gaps.

Credit each finding to its original authors ("A and B", "A et al."), linking the primary post. An aggregator is the venue, not the author.

## 6. Write-up

Item in the internal `docs/ngcc-submission-bugs.md` (local only, git-excluded: never commit it or cite it from committed files; rate crash/availability bugs in candidate code Low unless they leak secrets or allow forgery, and treat a prior report of the same root bug as already reported): Candidate (id, archive) · Affected (instances, tree, unchecked) · Severity · Security-critical (criterion) · Description (incl. what the submitter's build sets) · Evidence: `file:line` + short quote, re-checked against the archive · Reproduce (row, the change that flips it) · Suggested fix · Prior reports (searched what, or authors, venue, date, URL). Plus a summary-table row with Confidence.

Site notes stay neutral ("cause unknown", "did not finish"). Defect claims stay in the unpublished report; Thom discloses.

## Red flags: stop

"No build file ships" (checked the archive?) · "only the test driver" / "documented build is fine" (still security-critical) · "obviously new" (search first) · "credit ngcc.dev" (credit authors) · "retry in the harness" (wrapper or DNF) · "use their timings" (find the missing define).
