# Benchmark expansion plan

Add all round-3 schemes + classic (OpenSSL) baselines to the benchmark framework.
All parameter sets, including broken ones. SQIsign: original implementation.

## Classic schemes (OpenSSL)

`schemes/classic/` — no submodule, link system libcrypto via pkg-config.

- [x] RSA-2048
- [x] RSA-3072
- [x] RSA-4096
- [x] ECDSA-P256
- [x] ECDSA-P384
- [x] ECDSA-P521
- [x] Ed25519
- [x] Ed448

## HAWK (2 param sets)

Category: Lattice (LIP). Source: https://github.com/hawk-sign/dev (AVX2 impl).

- [x] Locate canonical GitHub repo
- [x] Add submodule
- [x] Write shims + Makefile
  - [x] HAWK-512
  - [x] HAWK-1024

## MAYO (4 param sets)

Category: Multivariate. Source: https://github.com/PQCMayo/MAYO-C (AVX2 + AES-NI).

- [x] Locate canonical GitHub repo
- [x] Add submodule
- [x] Write shims + Makefile
  - [x] MAYO-one
  - [x] MAYO-two  (broken: Wedge attack)
  - [x] MAYO-three
  - [x] MAYO-five

## FAEST (12 param sets)

Category: Symmetric (VOLE-in-the-Head). Source: TBD.

- [ ] Locate canonical GitHub repo
- [ ] Add submodule
- [ ] Write shim template + params.tsv + gen_shims.py + Makefile
  - [ ] FAEST-128s
  - [ ] FAEST-128f
  - [ ] FAEST-EM-128s
  - [ ] FAEST-EM-128f
  - [ ] FAEST-192s
  - [ ] FAEST-192f
  - [ ] FAEST-EM-192s
  - [ ] FAEST-EM-192f
  - [ ] FAEST-256s
  - [ ] FAEST-256f
  - [ ] FAEST-EM-256s
  - [ ] FAEST-EM-256f

## SDitH (6 param sets)

Category: MPC-in-the-Head. Source: TBD.

- [ ] Locate canonical GitHub repo
- [ ] Add submodule
- [ ] Write shim template + params.tsv + gen_shims.py + Makefile
  - [ ] SDitH2-L1-gf2-short
  - [ ] SDitH2-L1-gf2-fast
  - [ ] SDitH2-L3-gf2-short
  - [ ] SDitH2-L3-gf2-fast
  - [ ] SDitH2-L5-gf2-short
  - [ ] SDitH2-L5-gf2-fast

## SNOVA (11 param sets)

Category: Multivariate. Source: TBD.
Note: most param sets broken by Wedge product attack (ePrint 2026/237).

- [ ] Locate canonical GitHub repo
- [ ] Add submodule
- [ ] Write shim template + params.tsv + gen_shims.py + Makefile
  - [ ] SNOVA-(24,5,4)
  - [ ] SNOVA-(24,5,5)   (broken)
  - [ ] SNOVA-(25,8,3)   (broken)
  - [ ] SNOVA-(29,6,5)
  - [ ] SNOVA-(37,8,4)
  - [ ] SNOVA-(37,17,2)  (broken)
  - [ ] SNOVA-(49,11,3)
  - [ ] SNOVA-(56,25,2)
  - [ ] SNOVA-(60,10,4)
  - [ ] SNOVA-(66,15,3)
  - [ ] SNOVA-(75,33,2)

## UOV (8 param sets)

Category: Multivariate. Source: TBD.
Note: most param sets broken by Intersection attack (ePrint 2026/298).

- [ ] Locate canonical GitHub repo
- [ ] Add submodule
- [ ] Write shim template + params.tsv + gen_shims.py + Makefile
  - [ ] UOV-Ip-pkc      (broken)
  - [ ] UOV-Ip-classic  (broken)
  - [ ] UOV-Is-pkc
  - [ ] UOV-Is-classic
  - [ ] UOV-III-pkc
  - [ ] UOV-III-classic
  - [ ] UOV-V-pkc
  - [ ] UOV-V-classic

## QR-UOV (12 param sets)

Category: Multivariate. Source: TBD (NTT lab).

- [ ] Locate canonical GitHub repo / source package
- [ ] Add submodule
- [ ] Write shim template + params.tsv + gen_shims.py + Makefile
  - [ ] QR-UOV-I-(127,156,54,3)
  - [ ] QR-UOV-I-(31,165,60,3)
  - [ ] QR-UOV-I-(31,600,70,10)
  - [ ] QR-UOV-I-(7,740,100,10)
  - [ ] QR-UOV-III-(127,228,78,3)
  - [ ] QR-UOV-III-(31,246,87,3)
  - [ ] QR-UOV-III-(31,890,100,10)
  - [ ] QR-UOV-III-(7,1100,140,10)
  - [ ] QR-UOV-V-(127,306,105,3)
  - [ ] QR-UOV-V-(31,324,114,3)
  - [ ] QR-UOV-V-(31,1120,120,10)
  - [ ] QR-UOV-V-(7,1490,190,10)

## MQOM (36 param sets)

Category: MPC-in-the-Head. Source: TBD.

- [ ] Locate canonical GitHub repo
- [ ] Add submodule
- [ ] Write shim template + params.tsv + gen_shims.py + Makefile
  - [ ] L1-gf2-short-3r
  - [ ] L1-gf2-short-5r
  - [ ] L1-gf16-short-3r
  - [ ] L1-gf16-short-5r
  - [ ] L1-gf256-short-3r
  - [ ] L1-gf256-short-5r
  - [ ] L1-gf2-fast-3r
  - [ ] L1-gf2-fast-5r
  - [ ] L1-gf16-fast-3r
  - [ ] L1-gf16-fast-5r
  - [ ] L1-gf256-fast-3r
  - [ ] L1-gf256-fast-5r
  - [ ] L3-gf2-short-3r
  - [ ] L3-gf2-short-5r
  - [ ] L3-gf16-short-3r
  - [ ] L3-gf16-short-5r
  - [ ] L3-gf256-short-3r
  - [ ] L3-gf256-short-5r
  - [ ] L3-gf2-fast-3r
  - [ ] L3-gf2-fast-5r
  - [ ] L3-gf16-fast-3r
  - [ ] L3-gf16-fast-5r
  - [ ] L3-gf256-fast-3r
  - [ ] L3-gf256-fast-5r
  - [ ] L5-gf2-short-3r
  - [ ] L5-gf2-short-5r
  - [ ] L5-gf16-short-3r
  - [ ] L5-gf16-short-5r
  - [ ] L5-gf256-short-3r
  - [ ] L5-gf256-short-5r
  - [ ] L5-gf2-fast-3r
  - [ ] L5-gf2-fast-5r
  - [ ] L5-gf16-fast-3r
  - [ ] L5-gf16-fast-5r
  - [ ] L5-gf256-fast-3r
  - [ ] L5-gf256-fast-5r

## SQIsign (3 param sets)

Category: Isogenies. Source: original implementation (TBD — sqisign-org/sqisign or similar).
Note: extremely slow — sign takes seconds. Set iters=3.

- [ ] Locate canonical GitHub repo (original, not SQIsign2D or SQIsign-UF)
- [ ] Add submodule
- [ ] Write shims + Makefile
  - [ ] SQIsign-I
  - [ ] SQIsign-III
  - [ ] SQIsign-V

## Implementation notes

### Shim generation (mandatory for all schemes)

All schemes use the shim template system — hand-written shim files are banned.
`gen_shims.py` lives at `bench/` level and is shared across all schemes.

Per-scheme layout:
```
schemes/<name>/
├── params.tsv          # TSV: file  name  pk  sk  sig  iters  <scheme-specific…>
├── shim_template.c     # C template; @COLNAME@ tokens substituted per row
├── .gitignore          # contains: *_shim.c  (generated files not committed)
└── Makefile
```

Makefile rules (must follow these conventions exactly):
1. `all:` must be the **first** explicit target in every sub-Makefile so that
   `$(MAKE) -C schemes/<name>` without an explicit goal builds everything.
2. Shim generation rule: `$(SHIM_FILES): shim_template.c params.tsv`
   → `python3 ../../gen_shims.py shim_template.c params.tsv .`
3. Add `rm -f $(SHIM_FILES)` to the `clean:` rule.

Parent `bench/Makefile` uses per-scheme stamp files (`schemes/<name>/.stamp`)
so that parallel `-j` doesn't race multiple recursive make invocations against
each other.

### Source strategy

For each scheme: prefer a maintained GitHub repo over the NIST submission package.
Use `git submodule add` to pin by commit. Fall back to NIST package only if no
public GitHub exists.

### Iteration counts

- Fast operations (lattice, multivariate sign/verify): default BENCH_ITER (1000)
- Slow sign (hash-based -s, FAEST, MPC-in-the-Head): 50–200
- SQIsign keygen/sign: 3
- UOV keygen (large key gen): may need reduction
