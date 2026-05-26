#!/usr/bin/env python3
"""Generate FAEST C headers/sources from upstream templates (replaces Meson)."""

import os, sys, re

if len(sys.argv) < 3:
    sys.exit(f"usage: {sys.argv[0]} <ref-dir> <out-dir>")

REFDIR = sys.argv[1]
OUTDIR = sys.argv[2]
os.makedirs(OUTDIR, exist_ok=True)

# (PARAM, PARAM_L, LAMBDA, Nst, Ske, R, Senc, BETA, ELL, Lke, Lenc,
#  TAU, W_GRIND, T_OPEN, SIG_SIZE, PK_SIZE, SK_SIZE, OWF_IN, OWF_OUT)
PARAMS = [
    ("128S",    "128s",    128, 4, 40,  10, 160, 1, 1280, 448,  832, 11, 7,  102,  4506, 32, 32, 16, 16),
    ("128F",    "128f",    128, 4, 40,  10, 160, 1, 1280, 448,  832, 16, 8,  110,  5924, 32, 32, 16, 16),
    ("EM_128S", "em_128s", 128, 4,  0,  10, 160, 1,  960, 128,  832, 11, 7,  103,  3906, 32, 32, 16, 16),
    ("EM_128F", "em_128f", 128, 4,  0,  10, 160, 1,  960, 128,  832, 16, 8,  112,  5060, 32, 32, 16, 16),
    ("192S",    "192s",    192, 4, 32,  12, 192, 2, 2496, 448, 1024, 16, 12, 162, 11260, 48, 40, 16, 32),
    ("192F",    "192f",    192, 4, 32,  12, 192, 2, 2496, 448, 1024, 24,  8, 163, 14948, 48, 40, 16, 32),
    ("EM_192S", "em_192s", 192, 6,  0,  12, 288, 1, 1728, 192, 1536, 16,  8, 162,  9340, 48, 48, 24, 24),
    ("EM_192F", "em_192f", 192, 6,  0,  12, 288, 1, 1728, 192, 1536, 24,  8, 176, 12380, 48, 48, 24, 24),
    ("256S",    "256s",    256, 4, 52,  14, 224, 2, 3104, 672, 1216, 22,  6, 245, 20696, 48, 48, 16, 32),
    ("256F",    "256f",    256, 4, 52,  14, 224, 2, 3104, 672, 1216, 32,  8, 246, 26548, 48, 48, 16, 32),
    ("EM_256S", "em_256s", 256, 8,  0,  14, 448, 1, 2688, 256, 2432, 22,  6, 218, 17984, 64, 64, 32, 32),
    ("EM_256F", "em_256f", 256, 8,  0,  14, 448, 1, 2688, 256, 2432, 32,  8, 234, 23476, 64, 64, 32, 32),
]


def subst(template, **tokens):
    result = template
    for k, v in tokens.items():
        result = result.replace(f"@{k}@", str(v))
    return result


def write(path, content):
    with open(path, "w") as f:
        f.write(content)
    print(f"  wrote {path}")


# --- config.h (feature flags for Linux x86-64) ---
write(f"{OUTDIR}/config.h", """\
#pragma once
/* Generated for Linux x86-64; edit for other platforms. */
#define HAVE_AESNI             1
#define HAVE_AVX2              1
#define HAVE_MM_LOADU_SI64     1
#define HAVE_SYS_RANDOM_H      1
#define HAVE_GETRANDOM         1
#define HAVE_ALIGNED_ALLOC     1
#define HAVE_POSIX_MEMALIGN    1
#define HAVE_EXPLICIT_BZERO    1
#define HAVE_MEMPCPY           1
""")

# --- parameters.h ---
lines = ["#pragma once\n",
         "/* Global security-level constants used by shared source files. */\n",
         "#define FAEST_128_LAMBDA  128\n",
         "#define FAEST_192_LAMBDA  192\n",
         "#define FAEST_256_LAMBDA  256\n",
         "\n"]
for (param, param_l, lam, nst, ske, r, senc, beta, ell, lke, lenc,
     tau, w_grind, t_open, sig, pk, sk, owf_in, owf_out) in PARAMS:
    p = f"FAEST_{param}"
    lines.append(f"/* {p} */\n")
    lines.append(f"#define {p}_LAMBDA           {lam}\n")
    lines.append(f"#define {p}_TAU              {tau}\n")
    lines.append(f"#define {p}_W_GRIND          {w_grind}\n")
    lines.append(f"#define {p}_T_OPEN           {t_open}\n")
    lines.append(f"#define {p}_ELL              {ell}\n")
    lines.append(f"#define {p}_Nst              {nst}\n")
    lines.append(f"#define {p}_Ske              {ske}\n")
    lines.append(f"#define {p}_R                {r}\n")
    lines.append(f"#define {p}_Senc             {senc}\n")
    lines.append(f"#define {p}_BETA             {beta}\n")
    lines.append(f"#define {p}_Lke              {lke}\n")
    lines.append(f"#define {p}_Lenc             {lenc}\n")
    lines.append(f"#define {p}_SIG_SIZE         {sig}\n")
    lines.append(f"#define {p}_PK_SIZE          {pk}\n")
    lines.append(f"#define {p}_SK_SIZE          {sk}\n")
    lines.append(f"#define {p}_OWF_INPUT_SIZE   {owf_in}\n")
    lines.append(f"#define {p}_OWF_OUTPUT_SIZE  {owf_out}\n")
    lines.append("\n")
write(f"{OUTDIR}/parameters.h", "".join(lines))

# --- per-variant headers and sources ---
h_tmpl = open(f"{REFDIR}/faest_param.h.in").read()
c_tmpl = open(f"{REFDIR}/faest_param.c.in").read()

for (param, param_l, lam, nst, ske, r, senc, beta, ell, lke, lenc,
     tau, w_grind, t_open, sig, pk, sk, owf_in, owf_out) in PARAMS:
    tokens = dict(
        PARAM=param, PARAM_L=param_l, LAMBDA=lam,
        PK_SIZE=pk, SK_SIZE=sk, SIG_SIZE=sig,
        OWF_INPUT_SIZE=owf_in, OWF_OUTPUT_SIZE=owf_out, ELL=ell,
    )
    write(f"{OUTDIR}/faest_{param_l}.h", subst(h_tmpl, **tokens))
    write(f"{OUTDIR}/faest_{param_l}.c", subst(c_tmpl, **tokens))

# --- faest_aes_128/192/256.c ---
aes_tmpl = open(f"{REFDIR}/faest_aes.c.in").read()
for sss in (128, 192, 256):
    write(f"{OUTDIR}/faest_aes_{sss}.c", subst(aes_tmpl, SSS=sss))

print("gen_faest.py: done")
