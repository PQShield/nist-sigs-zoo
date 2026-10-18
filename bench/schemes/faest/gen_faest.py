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
    ('faest_128s',    128, 4066),
    ('faest_128f',    128, 5170),
    ('faest_em_128s', 128, 3466),
    ('faest_em_128f', 128, 4170),
    ('faest_192s',    192, 9410),
    ('faest_192f',    192, 11738),
    ('faest_em_192s', 192, 7874),
    ('faest_em_192f', 192, 9818),
    ('faest_256s',    256, 16626),
    ('faest_256f',    256, 20856),
    ('faest_em_256s', 256, 14554),
    ('faest_em_256f', 256, 18084),
]


def params_type(name):
    return 'v3::' + name[:-1] + '_' + name[-1:]


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
