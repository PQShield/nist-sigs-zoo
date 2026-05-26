#!/usr/bin/env python3
"""Generate per-variant api.cpp/api.h from faest-arch-opt templates."""

import os
import sys

if len(sys.argv) < 3:
    sys.exit(f"usage: {sys.argv[0]} <ref-dir> <out-dir>")

REFDIR = sys.argv[1]
OUTDIR = sys.argv[2]
os.makedirs(OUTDIR, exist_ok=True)

VARIANTS = [
    ('faest_128s',    128, 4506),
    ('faest_128f',    128, 5924),
    ('faest_em_128s', 128, 3906),
    ('faest_em_128f', 128, 5060),
    ('faest_192s',    192, 11260),
    ('faest_192f',    192, 14948),
    ('faest_em_192s', 192, 9340),
    ('faest_em_192f', 192, 12380),
    ('faest_256s',    256, 20696),
    ('faest_256f',    256, 26548),
    ('faest_em_256s', 256, 17984),
    ('faest_em_256f', 256, 23476),
]


def params_type(name):
    return 'v2::' + name[:-1] + '_' + name[-1:]


def key_sizes(name, secpar):
    is_em = 'em' in name
    sk = (2 * secpar // 8) if is_em else (16 + secpar // 8)
    pk = (2 * secpar // 8) if is_em else (16 + 16 * ((secpar + 127) // 128))
    return pk, sk


def subst(template, **tokens):
    result = template
    for k, v in tokens.items():
        result = result.replace(f'%{k}%', str(v))
    return result


h_tmpl = open(f'{REFDIR}/api.h.in').read()
cpp_tmpl = open(f'{REFDIR}/api.cpp.in').read()

for (name, secpar, sig) in VARIANTS:
    pk, sk = key_sizes(name, secpar)
    toks = dict(
        PARAMSTYPE=params_type(name),
        SECRETKEYBYTES=sk,
        PUBLICKEYBYTES=pk,
        SIGBYTES=sig,
        VERSION=name,
    )
    for ext, tmpl in (('h', h_tmpl), ('cpp', cpp_tmpl)):
        path = f'{OUTDIR}/{name}_api.{ext}'
        with open(path, 'w') as f:
            f.write(subst(tmpl, **toks))
        print(f'  wrote {path}')

print('gen_faest.py: done')
